
/**
 * @file usb-callback.h
 * @desc Wrapping USB callbacks together
 * @author Quinn Parrott
 * @date 2020-02-13
 * Last Author:
 * Last Edited On:
 */

#ifndef usb_callback_h
#define usb_callback_h

#include "global.h"
#include "config.h"

#include "usb-standard.h"

/** Configuration descriptor for CDC interface. */
extern const struct usb_device_configuration_descriptor usb_config_descriptor;

/**
 * Callback for when configuration is enabled by host.
 */
extern void usb_enable_config_callback(void);

/**
 * Callback for when configuration is disabled by host.
 */
extern void usb_disable_config_callback(void);

/**
 *  Callback to handle class specific requests.
 *
 *  @param packet The setup packet containing the request
 *  @param response_length Pointer to where length of response will be placed
 *  @param response_buffer Pointer to where response buffer will be placed
 *
 *  @return 0 if successful, a non-zero value otherwise
 */
extern uint8_t usb_class_request_callback(struct usb_setup_packet *packet,
                                          uint16_t *response_length,
                                          const uint8_t **response_buffer);

#endif /* usb-callback_h */