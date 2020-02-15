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

/* Endpoint buffers */
static uint8_t out_buffer[USB_STORAGE_ENDPOINT_SIZE];

/* Buffer with GET MAX LUN Reponse */
static const uint8_t lun_response = 0;

uint8_t usb_storage_class_request_callback(struct usb_setup_packet *packet,
                                           uint16_t *response_length,
                                           const uint8_t **response_buffer)
{
    switch ((enum usb_storage_class_specific)packet->bRequest)
    {
    case USB_STORAGE_REQ_MAX_LUN:
        *response_length = 1;
        *response_buffer = (const uint8_t*)&lun_response;
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
}

static void data_in_complete(void)
{
}

void usb_storage_enable_config_callback(void)
{
#ifdef FALSE
    /* Enable endpoints for CDC ACM interface 0 */
    /* Start endpoints for interface 0 */
    usb_start_out(USB_CDC_NOTIFICATION_ENDPOINT_0, notification_buffers_g[0],
                  USB_CDC_NOTIFICATION_EP_SIZE);
    usb_start_out(USB_CDC_DATA_OUT_ENDPOINT_0, out_buffers_g[0],
                  USB_CDC_DATA_EP_SIZE);
#endif
    /* Enable in and out bulk endpoints for USB Mass Storage */
    usb_enable_endpoint_out(USB_ENDPOINT_OUT_STORAGE,
                            USB_ENDPOINT_SIZE_64, // TODO: Check if this does anything
                            USB_ENDPOINT_TYPE_BULK,
                            &data_out_complete);

    usb_enable_endpoint_in(USB_ENDPOINT_IN_STORAGE,
                           USB_ENDPOINT_SIZE_64, // TODO: Check if this does anything
                           USB_ENDPOINT_TYPE_BULK,
                           &data_in_complete);

    /* Start the first endpoint transaction */
    usb_start_out(USB_ENDPOINT_OUT_STORAGE,
                  out_buffer,
                  USB_STORAGE_ENDPOINT_SIZE);
}

void usb_storage_disable_config_callback(void)
{
    usb_disable_endpoint_out(USB_ENDPOINT_OUT_STORAGE);
    usb_disable_endpoint_in(USB_ENDPOINT_IN_STORAGE);
}