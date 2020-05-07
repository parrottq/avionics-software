/**
 * @file usb-storage-state.h
 * @desc Internal usb-storage.h and scsi.h state. Used to store state across transactions
 * @author Quinn Parrott
 * @date 2020-05-07
 */

#ifndef usb_storage_state_h
#define usb_storage_state_h

#include "global.h"

enum usb_storage_mode_type
{
    /* Send data from storage_data_buffer but does not terminate it */
    SEND_CONTINUE,
    /* Send data from storage_data_buffer and terminate it */
    SEND_DONE,
    /* Receive data to storage_data_buffer */
    RECEIVE,
    /* Receive data to storage_command_buffer */
    NEXT_COMMAND,
};

struct usb_storage_state
{
    /* The function to call once the data transfer is done */
    uint8_t (*next_callback)(struct usb_storage_state *);

    /* Amount of residue */
    uint16_t residual_bytes;

    /* What buffer was filled */
    enum usb_storage_mode_type mode;

    /* The buffer data is received and sent in */
    uint8_t *const send_buffer;

    struct usb_storage_command_block_wrapper *const received_usb_command;

    union scsi_command_descriptor_block *const received_scsi_command;

    /* Number of bytes received */
    const uint16_t received_byte_count;

    /* Length of the current partial packet */
    uint16_t usb_packet_length;
};

// TODO: Can this nesting be done better
struct usb_storage_persistent_state
{
    /* The function to call once the data transfer is done */
    uint8_t (*next_callback)(struct usb_storage_state *);

    /* Amount of residue */
    uint16_t residual_bytes;

    /* What buffer was filled */
    enum usb_storage_mode_type mode;
};

#endif /* usb_storage_state_h */