/**
 * @file usb-storage.h
 * @desc USB Mass Storage implementation
 * @author Quinn Parrott
 * @date 2020-02-14
 * Last Author:
 * Last Edited On:
 */

#ifndef usb_storage_h
#define usb_storage_h

#include "global.h"

#include "usb-standard.h"

#define USB_STORAGE_MAX_OUT_BUFFER USB_ENDPOINT_SIZE_32
#define USB_STORAGE_MAX_IN_BUFFER USB_ENDPOINT_SIZE_64
/**
 *  Callback to handle class specific requests.
 *
 *  @param packet The setup packet containing the request
 *  @param response_length Pointer to where length of response will be placed
 *  @param response_buffer Pointer to where response buffer will be placed
 *
 *  @return 0 if successful, a non-zero value otherwise
 */
extern uint8_t usb_storage_class_request_callback(struct usb_setup_packet *packet,
                                                  uint16_t *response_length,
                                                  const uint8_t **response_buffer);

/**
 *  Callback for when mass storage configuration is enabled by host.
 */
extern void usb_storage_enable_config_callback(void);

/**
 *  Callback for when mass storage configuration is disabled by host.
 */
extern void usb_storage_disable_config_callback(void);

/**
 * Handle SCSI commands from host
 *
 * @param cdb_buffer SCSI Command Descriptor Block buffer
 * @param cdb_size Size of the SCSI command
 * @param reply_length The length of the contents reply
 *
 * @return 0 if successful, non-zero otherwise
 */
extern uint8_t usb_storage_scsi_host_handler(uint8_t *cdb_buffer, uint8_t cdb_size);

#endif /* usb_storage_h */