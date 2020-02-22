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

#define CLAMP_MAX(value, max_value) (value > max_value ? max_value : value)

/* Endpoint buffer from host (received) */
static uint8_t out_buffer[USB_STORAGE_MAX_OUT_BUFFER];

/* Endpoint buffer to host (sending) */
static uint8_t in_buffer[USB_STORAGE_BLOCK_SIZE];

/* USB Mass Storage packet header */
static struct usb_storage_command_block_wrapper *received_command_wrapper;

/* SCSI Command Descriptor Block request from host */
static struct usb_storage_command_descriptor_block_16 *received_scsi_command;

/* SCSI command callback */
static uint8_t (*scsi_command_callback)(uint16_t *);

/* Amount of residue */
static uint16_t residual_bytes;

/* SCSI command callbacks */
static uint8_t scsi_read_capacity_callback(uint16_t *packet_length);
static uint8_t scsi_inquiry_callback(uint16_t *packet_length);
static uint8_t usb_status_callback(uint16_t *packet_length, uint8_t status);
static uint8_t usb_status_success_callback(uint16_t *packet_length);
static uint8_t usb_status_failed_callback(uint16_t *packet_length);
static uint8_t scsi_mode_sense_callback(uint16_t *packet_length);

/* Other headers */
static void data_out_complete(uint16_t length);
static void data_in_complete(void);
static uint8_t scsi_load_into_received(struct usb_storage_command_descriptor_block_16 *scsi_command, uint8_t cdb_size);

uint8_t usb_storage_class_request_callback(struct usb_setup_packet *packet,
                                           uint16_t *response_length,
                                           const uint8_t **response_buffer)
{
    switch ((enum usb_storage_class_specific)packet->bRequest)
    {
    case USB_STORAGE_REQ_MAX_LUN:
        *response_length = 1;
        static const uint8_t lun_response = 0;
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
    /* USB Mass Storage level command wrapper */
    received_command_wrapper = (struct usb_storage_command_block_wrapper *)&out_buffer;

    /* If signature does not match or scsi parsing fails -> status failed */
    if (received_command_wrapper->signature != USB_STORAGE_SIGNATURE ||
        usb_storage_scsi_host_handler(
            (uint8_t *)out_buffer + sizeof(struct usb_storage_command_block_wrapper),
            received_command_wrapper->SCSILength) != 0)
    {
        /* SCSI handling failed */
        scsi_command_callback = usb_status_failed_callback;
    }

    /* Reset the number of proccesed */
    residual_bytes = 0;

    /* Start handling transaction */
    data_in_complete();
}

static void data_in_complete(void)
{
    /* Status command has been sent, wait for next */
    if (scsi_command_callback == 0)
    {
        usb_start_out(USB_ENDPOINT_OUT_STORAGE,
                      out_buffer,
                      USB_STORAGE_MAX_OUT_BUFFER);
        return;
    }

    static uint16_t usb_packet_length = 0;
    if (scsi_command_callback(&usb_packet_length) != 0)
    {
        /* Error, status failed callback will finish the transaction */
        scsi_command_callback = usb_status_failed_callback;
        // usb status callbacks never fail so no infinite recursion is possible
        data_in_complete();
    }
    else
    {
        /* If zero it's a status packet which does not need to be clamped */
        if (received_command_wrapper->dataTransferLength > 0)
        {
            /* Clamp packet length to the requested data length */
            usb_packet_length = CLAMP_MAX(usb_packet_length, received_command_wrapper->dataTransferLength);

            /* Substact from total requested length left */
            received_command_wrapper->dataTransferLength -= usb_packet_length;
        }

        usb_start_in(USB_ENDPOINT_IN_STORAGE, in_buffer, usb_packet_length, 1);
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
                           USB_STORAGE_MAX_IN_BUFFER,
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

static uint8_t scsi_load_into_received(struct usb_storage_command_descriptor_block_16 *scsi_command, uint8_t cdb_size)
{
    /* Point received to buffer */
    received_scsi_command = scsi_command;
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
        struct usb_storage_command_descriptor_block_6 *cdb6 = (struct usb_storage_command_descriptor_block_6 *)scsi_command;
        received_scsi_command->control = cdb6->control;
        received_scsi_command->options2 = 0;
        received_scsi_command->length = cdb6->length;
        received_scsi_command->logicalBlockAddress = (uint64_t)cdb6->logicalBlockAddress;
        received_scsi_command->options = cdb6->options;
        received_scsi_command->opcode = cdb6->opcode;
        break;
    case 10:;
        struct usb_storage_command_descriptor_block_10 *cdb10 = (struct usb_storage_command_descriptor_block_10 *)scsi_command;
        received_scsi_command->control = cdb10->control;
        received_scsi_command->options2 = cdb10->options2;
        received_scsi_command->length = cdb10->length >> 8;
        received_scsi_command->logicalBlockAddress = (uint64_t)cdb10->logicalBlockAddress;
        received_scsi_command->options = cdb10->options;
        received_scsi_command->opcode = cdb10->opcode;
        break;
    case 12:;
        struct usb_storage_command_descriptor_block_12 *cdb12 = (struct usb_storage_command_descriptor_block_12 *)scsi_command;
        received_scsi_command->control = cdb12->control;
        received_scsi_command->options2 = cdb12->options2;
        received_scsi_command->length = cdb12->length;
        received_scsi_command->logicalBlockAddress = (uint64_t)cdb12->logicalBlockAddress;
        received_scsi_command->options = cdb12->options;
        received_scsi_command->opcode = cdb12->opcode;
        break;
    case 16:
        received_scsi_command = (struct usb_storage_command_descriptor_block_16 *)scsi_command;
        break;
    default:
        return 1;
    }
    return 0;
}

uint8_t usb_storage_scsi_host_handler(uint8_t *cdb_buffer, uint8_t cdb_size)
{
    /* Recast any sized command descriptor block into biggest for easier handling */
    if (scsi_load_into_received((struct usb_storage_command_descriptor_block_16 *)cdb_buffer, cdb_size) != 0)
    {
        return 1;
    }

    switch (received_scsi_command->opcode)
    {
    case SCSI_OPCODE_FORMAT_UNIT:
        // Not implemented
        return 1;
        break;
    case SCSI_OPCODE_INQUIRY:
        scsi_command_callback = scsi_inquiry_callback;
        break;
    case SCSI_OPCODE_REQUEST_SENSE:
        // Not implemented
        return 1;
        break;
    case SCSI_OPCODE_READ_CAPACITY:
        scsi_command_callback = scsi_read_capacity_callback;
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
    case SCSI_OPCODE_TEST_UNIT_READY:
        /* Device is always ready */
        scsi_command_callback = usb_status_success_callback;
        break;
    case SCSI_MODE_SENSE_6:
        scsi_command_callback = scsi_mode_sense_callback;
        break;
    case SCSI_MEDIUM_REMOVAL:
        scsi_command_callback = usb_status_success_callback;
        break;
    case SCSI_SYNC_CACHE_10:
        scsi_command_callback = usb_status_success_callback;
        break;
    default:
        return 1;
    }
    return 0;
}

static uint8_t usb_status_failed_callback(uint16_t *packet_length)
{
    return usb_status_callback(packet_length, 1);
}

static uint8_t usb_status_success_callback(uint16_t *packet_length)
{
    return usb_status_callback(packet_length, 0);
}

static uint8_t usb_status_callback(uint16_t *packet_length, uint8_t status)
{
    /* Clamp padding to not exceed buffer length */
    uint16_t padding_length = CLAMP_MAX(received_command_wrapper->dataTransferLength, USB_STORAGE_BLOCK_SIZE);

    /* Check if there is still padding left */
    if (received_command_wrapper->dataTransferLength > 0)
    {
        /* Clear buffer */
        for (uint16_t i = 0; i < padding_length; i++)
        {
            in_buffer[i] = 0;
        }

        /* Added to residual byte count */
        residual_bytes += padding_length;

        /* Length of the padding returned */
        *packet_length = padding_length;
    }
    else
    {
        /* No padding left to send, send status */
        struct usb_storage_command_status_wrapper *status_wrapper = (struct usb_storage_command_status_wrapper *)in_buffer;
        status_wrapper->signature = USB_STORAGE_SIGNATURE;
        status_wrapper->tag = received_command_wrapper->tag;
        status_wrapper->residue = residual_bytes;
        status_wrapper->status = status;

        *packet_length = sizeof(struct usb_storage_command_status_wrapper);

        /* Done transaction */
        scsi_command_callback = 0;
    }

    // usb_status_callback cannot fail because it is the fallback for other commands failing
    return 0;
}

static uint8_t scsi_inquiry_callback(uint16_t *packet_length)
{
    struct scsi_inquiry_reply *inquiry_reply = (struct scsi_inquiry_reply *)in_buffer;
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

    // TODO: Improve this block
    // Device info
    inquiry_reply->vendorID[0] = 'C';
    inquiry_reply->vendorID[1] = 'U';
    inquiry_reply->vendorID[2] = 'S';
    inquiry_reply->vendorID[3] = 'p';
    inquiry_reply->vendorID[4] = 'a';
    inquiry_reply->vendorID[5] = 'c';
    inquiry_reply->vendorID[6] = 'e';
    inquiry_reply->vendorID[7] = ' ';

    for (uint8_t i = 0; i < 16; i++)
    {
        inquiry_reply->productID[i] = ' ';
    }
    inquiry_reply->productID[0] = 'M';
    inquiry_reply->productID[1] = 'C';
    inquiry_reply->productID[2] = 'U';

    inquiry_reply->productRevisionLevel[0] = '0';
    inquiry_reply->productRevisionLevel[1] = '0';
    inquiry_reply->productRevisionLevel[2] = '0';
    inquiry_reply->productRevisionLevel[3] = '1';

    *packet_length = sizeof(struct scsi_inquiry_reply);
    scsi_command_callback = usb_status_success_callback;
    return 0;
}

static uint8_t scsi_read_capacity_callback(uint16_t *packet_length)
{
    struct scsi_read_capacity_10_reply *read_capacity_reply = (struct scsi_read_capacity_10_reply *)in_buffer;
    // TODO: Actually make this more than 1
    /* These need to be big endian hence __REV */
    read_capacity_reply->logicalBlockAddress = __REV(100);
    read_capacity_reply->blockLength = __REV(USB_STORAGE_BLOCK_SIZE);

    *packet_length = sizeof(struct scsi_read_capacity_10_reply);
    scsi_command_callback = usb_status_success_callback;
    return 0;
}

static uint8_t scsi_mode_sense_callback(uint16_t *packet_length)
{
    struct scsi_mode_sense_reply *mode_sense_reply = (struct scsi_mode_sense_reply *)in_buffer;

    /* Size excluding itself */
    mode_sense_reply->modeDataLength = sizeof(struct scsi_mode_sense_reply) - 1;
    /* Header */
    mode_sense_reply->mediumType = 0;
    mode_sense_reply->reserved1 = 0;
    mode_sense_reply->blockDescriptorLength = 0;

    /* Cache Mode Page */
    mode_sense_reply->cachePageCode = 0x8;
    mode_sense_reply->cachePS = 0;
    mode_sense_reply->cacheSPF = 0;
    mode_sense_reply->cachePageLength = 18;
    mode_sense_reply->options1 = 0b100;
    mode_sense_reply->writeRetentionPriority = 0;
    mode_sense_reply->readRetentionPriority = 0;
    mode_sense_reply->disablePrefetchExceeds = 0;
    mode_sense_reply->minimumPrefetch = 0;
    mode_sense_reply->maximumPrefetch = 0;
    mode_sense_reply->maximumPrefetchCeiling = 0;
    mode_sense_reply->options2 = 0;
    mode_sense_reply->numberCache = 0;
    mode_sense_reply->cacheSegmentSize = 0;
    mode_sense_reply->reserved2 = 0;
    mode_sense_reply->obsolete1 = 0;

    /* Informational Exceptions Control Mode Page */
    mode_sense_reply->exceptPageCode = 0x1c;
    mode_sense_reply->exceptSPF = 0;
    mode_sense_reply->exceptPS = 0;
    mode_sense_reply->exceptPageLength = 10;
    mode_sense_reply->options3 = 0;
    mode_sense_reply->MRIE = 0;
    mode_sense_reply->reserved3 = 0;
    mode_sense_reply->intervalTime = 0;
    mode_sense_reply->reportCount = 0;

    *packet_length = received_command_wrapper->dataTransferLength;
    scsi_command_callback = usb_status_success_callback;
    return 0;
}
