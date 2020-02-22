
/**
 * @file usb-address.h
 * @desc Addresses of all the endpoints and interfaces
 * @author Quinn Parrott
 * @date 2020-02-13
 * Last Author:
 * Last Edited On:
 */

#ifndef usb_address_h
#define usb_address_h

#include "config.h"

/* USB interface constants */
enum
{
#ifdef ENABLE_USB_CDC_PORT_0
    USB_INTERFACE_CDC_CONTROL_0,
    USB_INTERFACE_CDC_DATA_0,
#endif
#ifdef ENABLE_USB_CDC_PORT_1
    USB_INTERFACE_CDC_CONTROL_1,
    USB_INTERFACE_CDC_DATA_1,
#endif
#ifdef ENABLE_USB_CDC_PORT_2
    USB_INTERFACE_CDC_CONTROL_2,
    USB_INTERFACE_CDC_DATA_2,
#endif
#ifdef ENABLE_USB_STORAGE
    USB_INTERFACE_STORAGE,
#endif

    /* Number of interfaces */
    USB_INTERFACE_COUNT
};

/* USB endpoints */
enum
{
    USB_ENDPOINT_IN_ZERO,
#ifdef ENABLE_USB_CDC_PORT_0
    USB_CDC_NOTIFICATION_ENDPOINT_0,
    USB_CDC_DATA_IN_ENDPOINT_0,
    USB_CDC_DATA_OUT_ENDPOINT_0,
#endif
#ifdef ENABLE_USB_CDC_PORT_1
    USB_CDC_NOTIFICATION_ENDPOINT_1,
    USB_CDC_DATA_IN_ENDPOINT_1,
    USB_CDC_DATA_OUT_ENDPOINT_1,
#endif
#ifdef ENABLE_USB_CDC_PORT_2
    USB_CDC_NOTIFICATION_ENDPOINT_2,
    USB_CDC_DATA_IN_ENDPOINT_2,
    USB_CDC_DATA_OUT_ENDPOINT_2,
#endif
#ifdef ENABLE_USB_STORAGE
    USB_ENDPOINT_IN_STORAGE,
    USB_ENDPOINT_OUT_STORAGE,
#endif

    /* Number of endpoints */
    USB_ENDPOINT_COUNT
};

#endif /* usb-address_h */