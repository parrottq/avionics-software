/**
 * @file usb-callback.c
 * @desc Wrapping USB callbacks together
 * @author Quinn Parrott
 * @date 2020-02-13
 * Last Author:
 * Last Edited On:
 */

#include "usb-callback.h"

#include "usb.h"
#include "usb-cdc.h"
#include "usb-storage.h"

void usb_enable_config_callback(void)
{
    usb_cdc_enable_config_callback();
}

void usb_disable_config_callback(void)
{
    usb_cdc_disable_config_callback();
}

uint8_t usb_class_request_callback(struct usb_setup_packet *packet,
                                   uint16_t *response_length,
                                   const uint8_t **response_buffer)
{
    uint8_t resp = 0;
    /* CDC specific requests */
    resp = usb_cdc_class_request_callback(packet, response_length, response_buffer);
    if (resp == 0)
    {
        return 0;
    }

    /* Mass Storage specific requests */
    resp = usb_storage_class_request_callback(packet, response_length, response_buffer);
    if (resp == 0)
    {
        return 0;
    }

    // Not a class specific request (that's supported)
    return 1;
}