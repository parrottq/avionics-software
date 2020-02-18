/**
 * @file usb-storage.c
 * @desc USB Mass Storage implementation
 * @author Quinn Parrott
 * @date 2020-02-14
 * Last Author:
 * Last Edited On:
 */

#include "usb-storage.h"

#include "global.h"

#include "usb.h"
#include "usb-address.h"

#include "usb-standard.h"
#include "usb-storage-standard.h"

#define USB_STORAGE_MAX_OUT_BUFFER 32

/* Endpoint buffer from host (received) */
static uint8_t out_buffer[USB_STORAGE_MAX_OUT_BUFFER];

/* Endpoint buffer to host (sending) */
static uint8_t in_buffer[USB_STORAGE_BLOCK_SIZE];

/* Buffer with GET MAX LUN Reponse */
static const uint8_t lun_response = 0;

/* USB Mass Storage packet header */
static struct usb_storage_command_block_wrapper *out_header;

/* SCSI Command Descriptor Block request from host */
static struct usb_storage_command_descriptor_block_16 *out_cdb;

uint8_t usb_storage_class_request_callback(struct usb_setup_packet *packet,
                                           uint16_t *response_length,
                                           const uint8_t **response_buffer)
{
    switch ((enum usb_storage_class_specific)packet->bRequest)
    {
    case USB_STORAGE_REQ_MAX_LUN:
        *response_length = 1;
        *response_buffer = (const uint8_t *)&lun_response;
        break;
    case USB_STORAGE_REQ_RESET:
        // Not supported
        return 1;
    default:
        // Unknown request, Request Error
        return 1;
    }

    return 0;
}

static void data_out_complete(uint16_t length)
{
    if (length != 31)
    {
        // TODO: Invalid command
    }
    if (out_header->signature != USB_STORAGE_SIGNATURE)
    {
        // TODO: Invalid command
    }

    /* USB Mass Storage level command wrapper */
    out_header = (struct usb_storage_command_block_wrapper *)&out_buffer;

    uint32_t scsi_reply_length = 0;
    if (usb_storage_scsi_host_handler(
            (uint8_t *)&out_buffer + sizeof(struct usb_storage_command_block_wrapper),
            out_header->SCSILength,
            &scsi_reply_length) != 0)
    {
        // TODO: Failed
        /*
        struct usb_storage_command_status_wrapper *status = (struct usb_storage_command_status_wrapper *)&in_buffer;
        status->signature = USB_STORAGE_SIGNATURE;
        status->tag = out_header->tag;
        status->residue = 0;
        status->status = 1;
        out_header = (void *)0;
        usb_start_in(USB_ENDPOINT_IN_STORAGE, in_buffer, sizeof(struct usb_storage_command_status_wrapper), 1);
        */
    }
    else
    {
        if (scsi_reply_length > 0)
        {
            usb_start_in(USB_ENDPOINT_IN_STORAGE, in_buffer, scsi_reply_length, 1);
        }
        else
        {
            out_header = (void *)0;
            usb_start_in(USB_ENDPOINT_IN_STORAGE, in_buffer, sizeof(struct usb_storage_command_status_wrapper), 1);
        }
    }
}

static void data_in_complete(void)
{
    if (!(out_header == 0 || (out_header->flags >> 7 & 1) == 0))
    {
        // Data transfer finished

        struct usb_storage_command_status_wrapper *command_status = (struct usb_storage_command_status_wrapper *)in_buffer;
        command_status->signature = USB_STORAGE_SIGNATURE;
        // Must be the same as the one in host request
        command_status->tag = out_header->tag;
        if (out_header->dataTransferLength > sizeof(in_buffer))
        {
            // The request data was too big for our buffer so indicate how much was omitted
            command_status->residue = out_header->dataTransferLength - sizeof(in_buffer);
        }
        else
        {
            command_status->residue = 0;
        }
        command_status->status = 0; // TODO: Error handling
        // Set as status command for the next time this endpoint is called, this is not sent to the host
        out_header->flags = 0;
        usb_start_in(USB_ENDPOINT_IN_STORAGE, in_buffer, sizeof(struct usb_storage_command_status_wrapper), 1);
    }
    else
    {
        // Status transfer finished
        usb_start_out(USB_ENDPOINT_OUT_STORAGE,
                      out_buffer,
                      USB_STORAGE_MAX_OUT_BUFFER);
    }
}

void usb_storage_enable_config_callback(void)
{
    /* Enable in and out bulk endpoints for USB Mass Storage */
    usb_enable_endpoint_out(USB_ENDPOINT_OUT_STORAGE,
                            USB_STORAGE_MAX_OUT_BUFFER,
                            USB_ENDPOINT_TYPE_BULK,
                            &data_out_complete);

    usb_enable_endpoint_in(USB_ENDPOINT_IN_STORAGE,
                           USB_STORAGE_BLOCK_SIZE,
                           USB_ENDPOINT_TYPE_BULK,
                           &data_in_complete);

    /* Start the first endpoint transaction */
    usb_start_out(USB_ENDPOINT_OUT_STORAGE,
                  out_buffer,
                  USB_STORAGE_MAX_OUT_BUFFER);
}

void usb_storage_disable_config_callback(void)
{
    usb_disable_endpoint_out(USB_ENDPOINT_OUT_STORAGE);
    usb_disable_endpoint_in(USB_ENDPOINT_IN_STORAGE);
}

uint8_t usb_storage_scsi_host_handler(uint8_t *cdb_buffer, uint8_t cdb_size, uint32_t *reply_length)
{
    // TODO: Make separate function
    out_cdb = (struct usb_storage_command_descriptor_block_16 *)cdb_buffer;
    /**
     * Instead of checking for each size variant of a command, cast to the biggest
     * since the bigger variant's memory is already allocated.
     *
     * Note: Since the same buffer is used, this overides the original command
     * structure. Values are assigned in reverse chronological order to not
     * overide unprocessed data.
     */
    switch (cdb_size)
    {
    case 6:; // semi-colon intentional, quirk in C
        struct usb_storage_command_descriptor_block_6 *cdb6 = (struct usb_storage_command_descriptor_block_6 *)cdb_buffer;
        out_cdb->control = cdb6->control;
        out_cdb->options2 = 0;
        out_cdb->length = cdb6->length;
        out_cdb->logicalBlockAddress = cdb6->logicalBlockAddress;
        out_cdb->options = cdb6->options;
        out_cdb->opcode = cdb6->opcode;
        break;
    case 10:;
        struct usb_storage_command_descriptor_block_10 *cdb10 = (struct usb_storage_command_descriptor_block_10 *)cdb_buffer;
        out_cdb->control = cdb10->control;
        out_cdb->options2 = 0;
        out_cdb->length = cdb10->length;
        out_cdb->logicalBlockAddress = cdb10->logicalBlockAddress;
        out_cdb->options = cdb10->options;
        out_cdb->opcode = cdb10->opcode;
        break;
    case 12:;
        struct usb_storage_command_descriptor_block_12 *cdb12 = (struct usb_storage_command_descriptor_block_12 *)cdb_buffer;
        out_cdb->control = cdb12->control;
        out_cdb->options2 = cdb12->options2;
        out_cdb->length = cdb12->length;
        out_cdb->logicalBlockAddress = cdb12->logicalBlockAddress;
        out_cdb->options = cdb12->options;
        out_cdb->opcode = cdb12->opcode;
        break;
    case 16:
        out_cdb = (struct usb_storage_command_descriptor_block_16 *)cdb_buffer;
        break;
    default:
        return 1;
    }

    switch (out_cdb->opcode)
    {
    case SCSI_OPCODE_FORMAT_UNIT:
        // Not implemented
        return 1;
        break;
    case SCSI_OPCODE_INQUIRY:;
        struct scsi_inquiry_reply *inquiry_reply = (struct scsi_inquiry_reply *)&in_buffer;
        // Direct Access Device
        inquiry_reply->peripheralDeviceType = 0;
        // Logical units
        inquiry_reply->peripheralQualifier = 0;
        // Removable?
        inquiry_reply->removableMedia = 1;
        // SCSI compliance (5 == SPC-3)
        inquiry_reply->version = 5;
        // 2 because all other versions are obsolete
        inquiry_reply->responseDataFormat = 2;
        // NormACA not supported
        inquiry_reply->NORMACA = 0;
        // HiSup not supported
        inquiry_reply->HISUP = 0;
        // Remaining length of the inquiry
        inquiry_reply->additionalLength = 31;
        // Stuff that's not supported
        inquiry_reply->SCCS = 0;
        inquiry_reply->ACC = 0;
        inquiry_reply->TPGS = 0;
        inquiry_reply->_3PC = 0;
        inquiry_reply->PROTECT = 0;
        inquiry_reply->ENCSERV = 0;
        inquiry_reply->MULTIP = 0;
        // Command Queuing: basic support
        inquiry_reply->BQUE = 1;
        inquiry_reply->CMDQUE = 0;
        // Zero all reserved and obsolete
        inquiry_reply->reserved1 = 0;
        inquiry_reply->reserved2 = 0;
        inquiry_reply->obsolete1 = 0;
        inquiry_reply->obsolete2 = 0;
        inquiry_reply->obsolete3 = 0;

        *reply_length = sizeof(struct scsi_inquiry_reply);
        break;
    case SCSI_OPCODE_REQUEST_SENSE:
        // Not implemented
        return 1;
        break;
    case SCSI_OPCODE_READ_CAPACITY:;
        struct scsi_read_capacity_10_reply *read_capacity_reply = (struct scsi_read_capacity_10_reply *)&in_buffer;
        // TODO: Actually make this more than 1
        read_capacity_reply->logicalBlockAddress = 1;
        read_capacity_reply->blockLength = USB_STORAGE_BLOCK_SIZE;

        *reply_length = sizeof(struct scsi_read_capacity_10_reply);
        break;
    case SCSI_OPCODE_READ_10:
        // Not implemented
        return 1;
        break;
    case SCSI_OPCODE_READ_16:
        // Not implemented
        return 1;
        break;
    case SCSI_OPCODE_REPORT_LUNS:
        // Not implemented
        return 1;
        break;
    case SCSI_OPCODE_SEND_DIAGNOSTIC:
        // Not implemented
        return 1;
        break;
    case SCSI_OPCODE_TEST_UNIT_READY:;
        struct usb_storage_command_status_wrapper *status = (struct usb_storage_command_status_wrapper *)&in_buffer;
        status->signature = USB_STORAGE_SIGNATURE;
        status->tag = out_header->tag;
        status->residue = 0;
        status->status = 0;
        break;
    }
    return 0;
}
