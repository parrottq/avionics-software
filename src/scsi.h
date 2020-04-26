/**
 * @file scsi.h
 * @desc SCSI commands
 * @author Quinn Parrott
 * @date 2020-04-26
 * @see usb_storage.c
 * Last Author:
 * Last Edited On:
 */

#ifndef scsi_h
#define scsi_h

#include "global.h"
#include "usb-storage.h"

/**
 * Set the proper SCSI command callback depending on the contents of
 * the USB command wrapper and SCSI command buffers.
 *
 * @param state Current USB Storage state
 *
 * @return 0 if successful, non-zero otherwise
 */
extern uint8_t scsi_set_command_callback(struct usb_storage_state *state);

/**
 * Callback to terminate a SCSI command successfully.
 *
 * @param state Current USB Storage state
 *
 * @return Always 0 (for success)
 */
extern uint8_t usb_status_success_callback(struct usb_storage_state *state);

/**
 * Callback to terminate a SCSI command with a command failure.
 *
 * @param state Current USB Storage state
 *
 * @return Always 0 (for success)
 */
extern uint8_t usb_status_failed_callback(struct usb_storage_state *state);

/* SCSI command callbacks */
extern uint8_t scsi_inquiry_callback(struct usb_storage_state *state);
extern uint8_t scsi_read_capacity_callback(struct usb_storage_state *state);
extern uint8_t scsi_read_10_callback(struct usb_storage_state *state);
extern uint8_t scsi_mode_sense_callback(struct usb_storage_state *state);

#endif /* scsi_h */
