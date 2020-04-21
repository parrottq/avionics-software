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
#include <string.h>

#include "usb.h"
#include "usb-address.h"

#include "usb-standard.h"
#include "usb-storage-standard.h"

#define CLAMP_MAX(value, max_value) (value > max_value ? max_value : value)

/* Persistent storage for commands from host */
static struct usb_storage_command_block_wrapper storage_usb_command;
static union scsi_command_descriptor_block storage_scsi_command;

/* Persistent information across usb transactions */
static struct usb_storage_persistent_state stored_state = {.mode = NEXT_COMMAND};

/* Buffer endpoints read from and write to */
static uint8_t storage_data_buffer[USB_STORAGE_BLOCK_SIZE * USB_STORAGE_BLOCK_COUNT];

/* Packet processing */
static void data_sent_complete(void);
static void data_received_complete(uint16_t length);
static void process_command(struct usb_storage_state *state);

/* USB status command callbacks */
static uint8_t usb_status_success_callback(struct usb_storage_state *state);
static uint8_t usb_status_failed_callback(struct usb_storage_state *state);
static uint8_t usb_status_callback(struct usb_storage_state *state, uint8_t status);

/* SCSI command callbacks */
static uint8_t scsi_read_10_callback(struct usb_storage_state *state);
static uint8_t scsi_inquiry_callback(struct usb_storage_state *state);
static uint8_t scsi_read_capacity_callback(struct usb_storage_state *state);
static uint8_t scsi_mode_sense_callback(struct usb_storage_state *state);

/* Other headers */
static void scsi_swap_command_endianess(struct usb_storage_state *state);

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

void usb_storage_enable_config_callback(void)
{
    /* Enable in and out bulk endpoints for USB Mass Storage */
    usb_enable_endpoint_out(USB_ENDPOINT_OUT_STORAGE,
                            USB_STORAGE_MAX_OUT_BUFFER,
                            USB_ENDPOINT_TYPE_BULK,
                            &data_received_complete);

    usb_enable_endpoint_in(USB_ENDPOINT_IN_STORAGE,
                           USB_STORAGE_MAX_IN_BUFFER,
                           USB_ENDPOINT_TYPE_BULK,
                           &data_sent_complete);

    /* Start the first endpoint transaction */
    usb_start_out(USB_ENDPOINT_OUT_STORAGE,
                  storage_data_buffer,
                  USB_STORAGE_MAX_OUT_BUFFER);
}

void usb_storage_disable_config_callback(void)
{
    usb_disable_endpoint_out(USB_ENDPOINT_OUT_STORAGE);
    usb_disable_endpoint_in(USB_ENDPOINT_IN_STORAGE);
}

static void data_sent_complete(void)
{
    data_received_complete(0);
}

static void data_received_complete(uint16_t length)
{
    struct usb_storage_state state = {
        /* Copy values from presistent state */
        .next_callback = stored_state.next_callback,
        .residual_bytes = stored_state.residual_bytes,
        .mode = stored_state.mode,

        /* Point to storage buffer */
        .send_buffer = storage_data_buffer,

        /* Point to USB and SCSI command blocks */
        .received_usb_command = &storage_usb_command,
        .received_scsi_command = &storage_scsi_command,
    };

    /* Check if triggered by send or received */
    if (length > 0)
    {
        /* Is this a new command, or incoming data */
        if (state.mode != RECEIVE)
        {
            /* SCSI command was received. This block needs to be the default state */

            /* Move command data into persistent command storage */
            memcpy(
                state.received_usb_command,
                state.send_buffer,
                sizeof(struct usb_storage_command_block_wrapper));
            memcpy(
                state.received_scsi_command,
                state.send_buffer + sizeof(struct usb_storage_command_block_wrapper),
                sizeof(union scsi_command_descriptor_block));

            /* SCSI is big endian, change to little endian */
            scsi_swap_command_endianess(&state);

            /* If signature does not match or scsi parsing fails -> status failed */
            if (state.received_usb_command->signature != USB_STORAGE_SIGNATURE ||
                usb_storage_scsi_host_handler(&state) != 0)
            {
                /* SCSI handling failed */
                state.next_callback = usb_status_failed_callback;
            }

            /* Reset the number of proccesed */
            state.residual_bytes = 0;
        }
    }

    /* Handle the command */
    process_command(&state);

    stored_state.next_callback = state.next_callback;
    stored_state.residual_bytes = state.residual_bytes;
    stored_state.mode = state.mode;
}

static void process_command(struct usb_storage_state *state)
{
    state->usb_packet_length = 0;

    /* Indicates if the data being sent is a continuation of the last packet */
    if (state->next_callback(state) != 0)
    {
        /* Error, status failed callback will finish the transaction */
        state->next_callback = usb_status_failed_callback;
        usb_status_failed_callback(state);
        return;
    }

    /* If zero it's a status packet which does not need to be clamped or accounted for */
    if (state->received_usb_command->dataTransferLength > 0)
    {
        /* Clamp packet length to the requested data length */
        state->usb_packet_length = CLAMP_MAX(state->usb_packet_length, state->received_usb_command->dataTransferLength);

        /* Substact from total requested length left */
        state->received_usb_command->dataTransferLength -= state->usb_packet_length;
    }

    switch (state->mode)
    {
    case SEND_CONTINUE:
        usb_start_in(USB_ENDPOINT_IN_STORAGE, state->send_buffer, state->usb_packet_length, 0);
        break;
    case SEND_DONE:
        usb_start_in(USB_ENDPOINT_IN_STORAGE, state->send_buffer, state->usb_packet_length, 1);
        break;
    case RECEIVE:
        usb_start_out(USB_ENDPOINT_OUT_STORAGE, state->send_buffer, USB_STORAGE_MAX_OUT_BUFFER);
        break;
    case NEXT_COMMAND:
        // TODO: Dedup with RECEIVE
        usb_start_out(USB_ENDPOINT_OUT_STORAGE, state->send_buffer, USB_STORAGE_MAX_OUT_BUFFER);
        break;
    }
}

static uint8_t usb_receive_next_command_callback(struct usb_storage_state *state)
{
    state->mode = NEXT_COMMAND;
    return 0;
}

static uint8_t usb_status_failed_callback(struct usb_storage_state *state)
{
    return usb_status_callback(state, 1);
}

static uint8_t usb_status_success_callback(struct usb_storage_state *state)
{
    return usb_status_callback(state, 0);
}

static uint8_t usb_status_callback(struct usb_storage_state *state, uint8_t status)
{
    /* Clamp padding to not exceed buffer length */
    uint16_t padding_length = CLAMP_MAX(state->received_usb_command->dataTransferLength, USB_STORAGE_BLOCK_SIZE * USB_STORAGE_BLOCK_COUNT);

    /* Check if there is still padding left */
    if (state->received_usb_command->dataTransferLength > 0)
    {
        /* Clear buffer */
        memset(state->send_buffer, 0, padding_length);

        /* Added to residual byte count */
        state->residual_bytes += padding_length;

        /* Length of the padding to send */
        state->usb_packet_length = padding_length;

        /* No data is left, terminate packet */
        if (state->received_usb_command->dataTransferLength > 0)
        {
            state->mode = SEND_CONTINUE;
        }
        else
        {
            state->mode = SEND_DONE;
        }
    }
    else
    {
        /* No padding left, send status */
        struct usb_storage_command_status_wrapper *status_wrapper = (struct usb_storage_command_status_wrapper *)state->send_buffer;
        status_wrapper->signature = USB_STORAGE_SIGNATURE;
        status_wrapper->tag = state->received_usb_command->tag;
        status_wrapper->residue = state->residual_bytes;
        status_wrapper->status = status;

        state->usb_packet_length = sizeof(struct usb_storage_command_status_wrapper);

        /* Done SCSI transaction */
        state->mode = SEND_DONE;
        state->next_callback = usb_receive_next_command_callback;
    }

    // usb_status_callback cannot fail because it is the fallback for other commands failing
    return 0;
}

static void scsi_swap_command_endianess(struct usb_storage_state *state)
{
    union scsi_command_descriptor_block *command_block = state->received_scsi_command;
    switch (state->received_usb_command->SCSILength)
    {
    case 6:
        command_block->_6.logicalBlockAddress = __REV16(command_block->_6.logicalBlockAddress);
        break;
    case 10:
        command_block->_10.logicalBlockAddress = __REV(command_block->_10.logicalBlockAddress);
        command_block->_10.length = __REV16(command_block->_10.length);
        break;
    case 12:
        command_block->_12.logicalBlockAddress = __REV(command_block->_12.logicalBlockAddress);
        command_block->_12.length = __REV(command_block->_12.length);
        break;
    case 16:;
        // 64 bit reverse is not natively supported
        uint64_t address = command_block->_16.logicalBlockAddress;
        uint64_t highAddress = __REV(address & 0xffffffff);
        uint64_t lowAddress = __REV((address >> 32) & 0xffffffff);
        command_block->_16.logicalBlockAddress = lowAddress | (highAddress << 32);

        command_block->_16.length = __REV(command_block->_16.length);
        break;
    }
}

uint8_t usb_storage_scsi_host_handler(struct usb_storage_state *state)
{
    /* Recast any sized command descriptor block into biggest for easier handling */

    // Since opcode is always in the same position any scsi command size can be used
    switch (state->received_scsi_command->_6.opcode)
    {
    case SCSI_OPCODE_FORMAT_UNIT:
        // Not implemented
        return 1;
        break;
    case SCSI_OPCODE_INQUIRY:
        state->next_callback = scsi_inquiry_callback;
        break;
    case SCSI_OPCODE_REQUEST_SENSE:
        // Not implemented
        return 1;
        break;
    case SCSI_OPCODE_READ_CAPACITY:
        state->next_callback = scsi_read_capacity_callback;
        break;
    case SCSI_OPCODE_READ_10:
        state->next_callback = scsi_read_10_callback;
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
        state->next_callback = usb_status_success_callback;
        break;
    case SCSI_MODE_SENSE_6:
        state->next_callback = scsi_mode_sense_callback;
        break;
    case SCSI_MEDIUM_REMOVAL:
        state->next_callback = usb_status_success_callback;
        break;
    case SCSI_SYNC_CACHE_10:
        state->next_callback = usb_status_success_callback;
        break;
    default:
        return 1;
    }
    return 0;
}

static uint8_t scsi_read_10_callback(struct usb_storage_state *state)
{
    /* Fill buffer with some numbers to distinguish it */
    uint8_t block_count = CLAMP_MAX(state->received_scsi_command->_10.length, USB_STORAGE_BLOCK_COUNT);
    for (uint8_t block_num = 0; block_num < block_count; block_num++)
    {
        uint8_t *offset_buffer = state->send_buffer + (USB_STORAGE_BLOCK_SIZE * block_num);
        uint64_t pos = state->received_scsi_command->_10.logicalBlockAddress;
        uint64_t sector = pos;
        if (sector != ~((uint64_t)0))
        {
            // Is part of the file sector is offset
            // TODO: Read meaningful data instead of numbers
            char ch = 0;
            memset(offset_buffer, ch, 512);
            memcpy(offset_buffer, &sector, sizeof(uint64_t));
        }
        /* Subtract from the number of blocks left */
        state->received_scsi_command->_10.length -= 1;

        /* Point to the next block */
        state->received_scsi_command->_10.logicalBlockAddress += 1;
    }

    /* Transfer length */
    state->usb_packet_length = USB_STORAGE_BLOCK_SIZE * block_count;

    if (state->received_scsi_command->_10.length == 0)
    {
        /* No blocks left, send success */
        state->next_callback = usb_status_success_callback;
        state->mode = SEND_DONE;
    }
    else
    {
        /* This is still one packet since not all data has been transferred */
        state->mode = SEND_CONTINUE;
    }

    return 0;
}

static uint8_t scsi_inquiry_callback(struct usb_storage_state *state)
{
    struct scsi_inquiry_reply *inquiry_reply = (struct scsi_inquiry_reply *)state->send_buffer;
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

    // Device info
    const char vendorID[10] = "CUInSpace";
    memcpy(inquiry_reply->vendorID, vendorID, sizeof(inquiry_reply->vendorID));
    const char productID[17] = "CarletonU Rocket";
    memcpy(inquiry_reply->productID, productID, sizeof(inquiry_reply->productID));
    const char productRevisionLevel[5] = "0001";
    memcpy(inquiry_reply->productRevisionLevel, productRevisionLevel, sizeof(inquiry_reply->productRevisionLevel));

    state->usb_packet_length = sizeof(struct scsi_inquiry_reply);
    state->next_callback = usb_status_success_callback;
    state->mode = SEND_DONE;
    return 0;
}

static uint8_t scsi_read_capacity_callback(struct usb_storage_state *state)
{
    struct scsi_read_capacity_10_reply *read_capacity_reply = (struct scsi_read_capacity_10_reply *)state->send_buffer;
    /* These need to be big endian hence __REV */
    read_capacity_reply->logicalBlockAddress = __REV(100);
    read_capacity_reply->blockLength = __REV(USB_STORAGE_BLOCK_SIZE);

    state->usb_packet_length = sizeof(struct scsi_read_capacity_10_reply);
    state->next_callback = usb_status_success_callback;
    state->mode = SEND_DONE;
    return 0;
}

static uint8_t scsi_mode_sense_callback(struct usb_storage_state *state)
{
    struct scsi_mode_sense_reply *mode_sense_reply = (struct scsi_mode_sense_reply *)state->send_buffer;

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

    state->usb_packet_length = state->received_usb_command->dataTransferLength;
    state->next_callback = usb_status_success_callback;
    // TODO: Set this as a default argument
    state->mode = SEND_DONE;
    return 0;
}
