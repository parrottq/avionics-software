/**
 * @file usb-storage.c
 * @desc USB Mass Storage implementation
 * @author Quinn Parrott
 * @date 2020-02-14
 */

#include "usb-storage.h"

#include <string.h>
#include "math_util.h"

#include "scsi-standard.h"
#include "usb-storage-standard.h"
#include "usb-storage-state.h"

#include "usb.h"
#include "usb-address.h"

#include "scsi.h"

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

static void swap_command_endianess(struct usb_storage_state *state)
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
        .error = stored_state.error,

        /* Point to storage buffer */
        .send_buffer = storage_data_buffer,

        /* Point to USB and SCSI command blocks */
        .received_usb_command = &storage_usb_command,
        .received_scsi_command = &storage_scsi_command,

        /* Copy the number of received bytes */
        .received_byte_count = length,
    };

    /* Check if triggered by send or received */
    if (state.received_byte_count > 0)
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
            swap_command_endianess(&state);

            /* If signature does not match or scsi parsing fails -> status failed */
            if (state.received_usb_command->signature != USB_STORAGE_COMMAND_BLOCK_WRAPPER_SIGNATURE ||
                scsi_set_command_callback(&state) != 0)
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
    stored_state.error = state.error;
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
    default:
        // RECEIVED and NEXT_COMMAND
        usb_start_out(USB_ENDPOINT_OUT_STORAGE, state->send_buffer, USB_STORAGE_BLOCK_SIZE * USB_STORAGE_BLOCK_COUNT);
        break;
    }
}
