/**
 * @file usb_descriptor.c
 * @desc USB CDC interface configuration descriptor
 * @author Samuel Dewan
 * @date 2019-12-27
 * Last Author: Quinn Parrott
 * Last Edited On: 2020-02-13
 */

#include "usb-callback.h"

#include "usb-storage-standard.h"
#include "usb-cdc-standard.h"
#include "usb-address.h"
#include "usb-cdc.h"
#include "usb-storage.h"

// Ignore warnings in this file about inefficient alignment
#pragma GCC diagnostic ignored "-Wattributes"
#pragma GCC diagnostic ignored "-Wpacked"

struct usb_device_configuration_descriptor {
    // Configuration
    struct usb_configuration_descriptor configuration;
#ifdef ENABLE_USB_CDC_PORT_0
    /* Virtual Serial Port 0 */
    // Communication Class Interface
    struct usb_interface_association_descriptor association_0;
    struct usb_interface_descriptor cdc_control_interface_0;
    struct usb_cdc_header_functional_descriptor cdc_header_0;
    struct usb_cdc_acm_functional_descriptor cdc_acm_desc_0;
    struct usb_cdc_union_functional_descriptor cdc_union_0;
    struct usb_cdc_call_management_functional_descriptor cdc_call_man_0;
    struct usb_endpoint_descriptor cdc_notification_endpoint_0;
    // Data Class Interface
    struct usb_interface_descriptor cdc_data_interface_0;
    struct usb_endpoint_descriptor cdc_data_in_endpoint_0;
    struct usb_endpoint_descriptor cdc_data_out_endpoint_0;
#endif
#ifdef ENABLE_USB_CDC_PORT_1
    /* Virtual Serial Port 1 */
    // Communication Class Interface
    struct usb_interface_association_descriptor association_1;
    struct usb_interface_descriptor cdc_control_interface_1;
    struct usb_cdc_header_functional_descriptor cdc_header_1;
    struct usb_cdc_acm_functional_descriptor cdc_acm_desc_1;
    struct usb_cdc_union_functional_descriptor cdc_union_1;
    struct usb_cdc_call_management_functional_descriptor cdc_call_man_1;
    struct usb_endpoint_descriptor cdc_notification_endpoint_1;
    // Data Class Interface
    struct usb_interface_descriptor cdc_data_interface_1;
    struct usb_endpoint_descriptor cdc_data_in_endpoint_1;
    struct usb_endpoint_descriptor cdc_data_out_endpoint_1;
#endif
#ifdef ENABLE_USB_CDC_PORT_2
    /* Virtual Serial Port 2 */
    // Communication Class Interface
    struct usb_interface_association_descriptor association_2;
    struct usb_interface_descriptor cdc_control_interface_2;
    struct usb_cdc_header_functional_descriptor cdc_header_2;
    struct usb_cdc_acm_functional_descriptor cdc_acm_desc_2;
    struct usb_cdc_union_functional_descriptor cdc_union_2;
    struct usb_cdc_call_management_functional_descriptor cdc_call_man_2;
    struct usb_endpoint_descriptor cdc_notification_endpoint_2;
    // Data Class Interface
    struct usb_interface_descriptor cdc_data_interface_2;
    struct usb_endpoint_descriptor cdc_data_in_endpoint_2;
    struct usb_endpoint_descriptor cdc_data_out_endpoint_2;
#endif
#ifdef ENABLE_USB_STORAGE
    /* USB Mass Storage Interface */
    struct usb_interface_association_descriptor association_storage;
    struct usb_interface_descriptor storage_interface;
    struct usb_endpoint_descriptor storage_in_endpoint;
    struct usb_endpoint_descriptor storage_out_endpoint;
#endif
} __attribute__((packed));

// Stop ignoring warnings about inefficient alignment
#pragma GCC diagnostic pop

__attribute__((__aligned__(4)))
const struct usb_device_configuration_descriptor usb_config_descriptor = {
    // Configuration
    .configuration = {
        .bLength = sizeof(struct usb_configuration_descriptor),
        .bDescriptorType = USB_DESC_TYPE_CONFIGURATION,
        .wTotalLength  = sizeof(struct usb_device_configuration_descriptor),
        .bNumInterfaces = USB_INTERFACE_COUNT,
        .bConfigurationValue = 1,
        .iConfiguration = 0,
        .bmAttributes.RESERVED = 1,
        .bMaxPower = 250
    },
#ifdef ENABLE_USB_CDC_PORT_0
    /* Virtual Serial Port 0 */
    .association_0 = {
        .bLength = sizeof(struct usb_interface_association_descriptor),
        .bDescriptorType = USB_DESC_TYPE_INTERFACE_ASSOCIATION,
        .bFirstInterface = USB_INTERFACE_CDC_CONTROL_0,
        .bInterfaceCount = 2,
        .bFunctionClass = USB_CDC_CLASS_CODE,
        .bFunctionSubClass = USB_CDC_SUBCLASS_ACM,
        .bFunctionProtocol = USB_CDC_COMM_PROTOCOL_AT_V250,
        .iFunction = 0
    },
    // Communication Class Interface
    .cdc_control_interface_0 = {
        .bLength = sizeof(struct usb_interface_descriptor),
        .bDescriptorType = USB_DESC_TYPE_INTERFACE,
        .bInterfaceNumber = USB_INTERFACE_CDC_CONTROL_0,
        .bAlternateSetting = 0,
        .bNumEndpoints = 1,
        .bInterfaceClass = USB_CDC_CLASS_CODE,
        .bInterfaceSubClass = USB_CDC_SUBCLASS_ACM,
        .bInterfaceProtocol = USB_CDC_COMM_PROTOCOL_AT_V250,
        .iInterface = 0
    },
    .cdc_header_0 = {
        .bLength = sizeof(struct usb_cdc_header_functional_descriptor),
        .bDescriptorType = USB_CDC_DESC_TYPE_CS_INTERFACE,
        .bDescriptorSubtype = USB_CDC_DESC_SUBTYPE_HEADER,
        .bcdCDC = 0x0110
    },
    .cdc_acm_desc_0 = {
        .bLength = sizeof(struct usb_cdc_acm_functional_descriptor),
        .bDescriptorType = USB_CDC_DESC_TYPE_CS_INTERFACE,
        .bDescriptorSubtype = USB_CDC_DESC_SUBTYPE_ACM,
        //.bmCapabilities.supports_line_coding = 0b1
        .bmCapabilities.raw = 0x6
    },
    .cdc_union_0 = {
        .bLength = sizeof(struct usb_cdc_union_functional_descriptor),
        .bDescriptorType = USB_CDC_DESC_TYPE_CS_INTERFACE,
        .bDescriptorSubtype = USB_CDC_DESC_SUBTYPE_UNION,
        .bControlInterface = USB_INTERFACE_CDC_CONTROL_0,
        .bSlaveInterface = USB_INTERFACE_CDC_DATA_0
    },
    .cdc_call_man_0 = {
        .bLength = sizeof(struct usb_cdc_call_management_functional_descriptor),
        .bDescriptorType = USB_CDC_DESC_TYPE_CS_INTERFACE,
        .bDescriptorSubtype = USB_CDC_DESC_SUBTYPE_CALL_MANAGMENT,
        .bmCapabilities.handles_call_management = 1,
        .bDataInterface = USB_INTERFACE_CDC_DATA_0
    },
    .cdc_notification_endpoint_0 = {
        .bLength = sizeof(struct usb_endpoint_descriptor),
        .bDescriptorType = USB_DESC_TYPE_ENDPOINT,
        .bEndpointAddress.direction = USB_DATA_TRANS_DEVICE_TO_HOST,
        .bEndpointAddress.endpoint_number = USB_CDC_NOTIFICATION_ENDPOINT_0,
        .bmAttributes.transfer_type = USB_TRANS_TYPE_INTERRUPT,
        .bmAttributes.sync_type = USB_SYNC_TYPE_NONE,
        .bmAttributes.usage_type = USB_USAGE_TYPE_DATA,
        .wMaxPacketSize = USB_CDC_NOTIFICATION_EP_SIZE,
        .bInterval = 8
    },
    // Data Class Interface
    .cdc_data_interface_0 = {
        .bLength = sizeof(struct usb_interface_descriptor),
        .bDescriptorType = USB_DESC_TYPE_INTERFACE,
        .bInterfaceNumber = USB_INTERFACE_CDC_DATA_0,
        .bAlternateSetting = 0,
        .bNumEndpoints = 2,
        .bInterfaceClass = USB_CDC_DATA_INTERFACE_CLASS,
        .bInterfaceSubClass = 0,
        .bInterfaceProtocol = 0,
        .iInterface = 0
    },
    .cdc_data_in_endpoint_0 = {
        .bLength = sizeof(struct usb_endpoint_descriptor),
        .bDescriptorType = USB_DESC_TYPE_ENDPOINT,
        .bEndpointAddress.direction = USB_DATA_TRANS_DEVICE_TO_HOST,
        .bEndpointAddress.endpoint_number = USB_CDC_DATA_IN_ENDPOINT_0,
        .bmAttributes.transfer_type = USB_TRANS_TYPE_BULK,
        .bmAttributes.sync_type = USB_SYNC_TYPE_NONE,
        .bmAttributes.usage_type = USB_USAGE_TYPE_DATA,
        .wMaxPacketSize = USB_CDC_DATA_EP_SIZE,
        .bInterval = 0
    },
    .cdc_data_out_endpoint_0 = {
        .bLength = sizeof(struct usb_endpoint_descriptor),
        .bDescriptorType = USB_DESC_TYPE_ENDPOINT,
        .bEndpointAddress.direction = USB_DATA_TRANS_HOST_TO_DEVICE,
        .bEndpointAddress.endpoint_number = USB_CDC_DATA_OUT_ENDPOINT_0,
        .bmAttributes.transfer_type = USB_TRANS_TYPE_BULK,
        .bmAttributes.sync_type = USB_SYNC_TYPE_NONE,
        .bmAttributes.usage_type = USB_USAGE_TYPE_DATA,
        .wMaxPacketSize = USB_CDC_DATA_EP_SIZE,
        .bInterval = 0
    },
#endif
#ifdef ENABLE_USB_CDC_PORT_1
    /* Virtual Serial Port 1 */
    .association_1 = {
        .bLength = sizeof(struct usb_interface_association_descriptor),
        .bDescriptorType = USB_DESC_TYPE_INTERFACE_ASSOCIATION,
        .bFirstInterface = USB_INTERFACE_CDC_CONTROL_1,
        .bInterfaceCount = 2,
        .bFunctionClass = USB_CDC_CLASS_CODE,
        .bFunctionSubClass = USB_CDC_SUBCLASS_ACM,
        .bFunctionProtocol = USB_CDC_COMM_PROTOCOL_AT_V250,
        .iFunction = 0
    },
    // Communication Class Interface
    .cdc_control_interface_1 = {
        .bLength = sizeof(struct usb_interface_descriptor),
        .bDescriptorType = USB_DESC_TYPE_INTERFACE,
        .bInterfaceNumber = USB_INTERFACE_CDC_CONTROL_1,
        .bAlternateSetting = 0,
        .bNumEndpoints = 1,
        .bInterfaceClass = USB_CDC_CLASS_CODE,
        .bInterfaceSubClass = USB_CDC_SUBCLASS_ACM,
        .bInterfaceProtocol = USB_CDC_COMM_PROTOCOL_AT_V250,
        .iInterface = 0
    },
    .cdc_header_1 = {
        .bLength = sizeof(struct usb_cdc_header_functional_descriptor),
        .bDescriptorType = USB_CDC_DESC_TYPE_CS_INTERFACE,
        .bDescriptorSubtype = USB_CDC_DESC_SUBTYPE_HEADER,
        .bcdCDC = 0x0120
    },
    .cdc_acm_desc_1 = {
        .bLength = sizeof(struct usb_cdc_acm_functional_descriptor),
        .bDescriptorType = USB_CDC_DESC_TYPE_CS_INTERFACE,
        .bDescriptorSubtype = USB_CDC_DESC_SUBTYPE_ACM,
        .bmCapabilities.supports_line_coding = 0b1
    },
    .cdc_union_1 = {
        .bLength = sizeof(struct usb_cdc_union_functional_descriptor),
        .bDescriptorType = USB_CDC_DESC_TYPE_CS_INTERFACE,
        .bDescriptorSubtype = USB_CDC_DESC_SUBTYPE_UNION,
        .bControlInterface = USB_INTERFACE_CDC_CONTROL_1,
        .bSlaveInterface = USB_INTERFACE_CDC_DATA_1
    },
    .cdc_call_man_1 = {
        .bLength = sizeof(struct usb_cdc_call_management_functional_descriptor),
        .bDescriptorType = USB_CDC_DESC_TYPE_CS_INTERFACE,
        .bDescriptorSubtype = USB_CDC_DESC_SUBTYPE_CALL_MANAGMENT,
        .bmCapabilities.handles_call_management = 1,
        .bDataInterface = USB_INTERFACE_CDC_DATA_1
    },
    .cdc_notification_endpoint_1 = {
        .bLength = sizeof(struct usb_endpoint_descriptor),
        .bDescriptorType = USB_DESC_TYPE_ENDPOINT,
        .bEndpointAddress.direction = USB_DATA_TRANS_DEVICE_TO_HOST,
        .bEndpointAddress.endpoint_number = USB_CDC_NOTIFICATION_ENDPOINT_1,
        .bmAttributes.transfer_type = USB_TRANS_TYPE_INTERRUPT,
        .bmAttributes.sync_type = USB_SYNC_TYPE_NONE,
        .bmAttributes.usage_type = USB_USAGE_TYPE_DATA,
        .wMaxPacketSize = USB_CDC_NOTIFICATION_EP_SIZE,
        .bInterval = 8
    },
    // Data Class Interface
    .cdc_data_interface_1 = {
        .bLength = sizeof(struct usb_interface_descriptor),
        .bDescriptorType = USB_DESC_TYPE_INTERFACE,
        .bInterfaceNumber = USB_INTERFACE_CDC_DATA_1,
        .bAlternateSetting = 0,
        .bNumEndpoints = 2,
        .bInterfaceClass = USB_CDC_DATA_INTERFACE_CLASS,
        .bInterfaceSubClass = 0,
        .bInterfaceProtocol = 0,
        .iInterface = 0
    },
    .cdc_data_in_endpoint_1 = {
        .bLength = sizeof(struct usb_endpoint_descriptor),
        .bDescriptorType = USB_DESC_TYPE_ENDPOINT,
        .bEndpointAddress.direction = USB_DATA_TRANS_DEVICE_TO_HOST,
        .bEndpointAddress.endpoint_number = USB_CDC_DATA_IN_ENDPOINT_1,
        .bmAttributes.transfer_type = USB_TRANS_TYPE_BULK,
        .bmAttributes.sync_type = USB_SYNC_TYPE_NONE,
        .bmAttributes.usage_type = USB_USAGE_TYPE_DATA,
        .wMaxPacketSize = USB_CDC_DATA_EP_SIZE,
        .bInterval = 0
    },
    .cdc_data_out_endpoint_1 = {
        .bLength = sizeof(struct usb_endpoint_descriptor),
        .bDescriptorType = USB_DESC_TYPE_ENDPOINT,
        .bEndpointAddress.direction = USB_DATA_TRANS_HOST_TO_DEVICE,
        .bEndpointAddress.endpoint_number = USB_CDC_DATA_OUT_ENDPOINT_1,
        .bmAttributes.transfer_type = USB_TRANS_TYPE_BULK,
        .bmAttributes.sync_type = USB_SYNC_TYPE_NONE,
        .bmAttributes.usage_type = USB_USAGE_TYPE_DATA,
        .wMaxPacketSize = USB_CDC_DATA_EP_SIZE,
        .bInterval = 0
    },
#endif
#ifdef ENABLE_USB_CDC_PORT_2
    /* Virtual Serial Port 2 */
    .association_2 = {
        .bLength = sizeof(struct usb_interface_association_descriptor),
        .bDescriptorType = USB_DESC_TYPE_INTERFACE_ASSOCIATION,
        .bFirstInterface = USB_INTERFACE_CDC_CONTROL_2,
        .bInterfaceCount = 2,
        .bFunctionClass = USB_CDC_CLASS_CODE,
        .bFunctionSubClass = USB_CDC_SUBCLASS_ACM,
        .bFunctionProtocol = USB_CDC_COMM_PROTOCOL_AT_V250,
        .iFunction = 0
    },
    // Communication Class Interface
    .cdc_control_interface_2 = {
        .bLength = sizeof(struct usb_interface_descriptor),
        .bDescriptorType = USB_DESC_TYPE_INTERFACE,
        .bInterfaceNumber = USB_INTERFACE_CDC_CONTROL_2,
        .bAlternateSetting = 0,
        .bNumEndpoints = 1,
        .bInterfaceClass = USB_CDC_CLASS_CODE,
        .bInterfaceSubClass = USB_CDC_SUBCLASS_ACM,
        .bInterfaceProtocol = USB_CDC_COMM_PROTOCOL_AT_V250,
        .iInterface = 0
    },
    .cdc_header_2 = {
        .bLength = sizeof(struct usb_cdc_header_functional_descriptor),
        .bDescriptorType = USB_CDC_DESC_TYPE_CS_INTERFACE,
        .bDescriptorSubtype = USB_CDC_DESC_SUBTYPE_HEADER,
        .bcdCDC = 0x0120
    },
    .cdc_acm_desc_2 = {
        .bLength = sizeof(struct usb_cdc_acm_functional_descriptor),
        .bDescriptorType = USB_CDC_DESC_TYPE_CS_INTERFACE,
        .bDescriptorSubtype = USB_CDC_DESC_SUBTYPE_ACM,
        .bmCapabilities.supports_line_coding = 0b1
    },
    .cdc_union_2 = {
        .bLength = sizeof(struct usb_cdc_union_functional_descriptor),
        .bDescriptorType = USB_CDC_DESC_TYPE_CS_INTERFACE,
        .bDescriptorSubtype = USB_CDC_DESC_SUBTYPE_UNION,
        .bControlInterface = USB_INTERFACE_CDC_CONTROL_2,
        .bSlaveInterface = USB_INTERFACE_CDC_DATA_2
    },
    .cdc_call_man_2 = {
        .bLength = sizeof(struct usb_cdc_call_management_functional_descriptor),
        .bDescriptorType = USB_CDC_DESC_TYPE_CS_INTERFACE,
        .bDescriptorSubtype = USB_CDC_DESC_SUBTYPE_CALL_MANAGMENT,
        .bmCapabilities.handles_call_management = 1,
        .bDataInterface = USB_INTERFACE_CDC_DATA_2
    },
    .cdc_notification_endpoint_2 = {
        .bLength = sizeof(struct usb_endpoint_descriptor),
        .bDescriptorType = USB_DESC_TYPE_ENDPOINT,
        .bEndpointAddress.direction = USB_DATA_TRANS_DEVICE_TO_HOST,
        .bEndpointAddress.endpoint_number = USB_CDC_NOTIFICATION_ENDPOINT_1,
        .bmAttributes.transfer_type = USB_TRANS_TYPE_INTERRUPT,
        .bmAttributes.sync_type = USB_SYNC_TYPE_NONE,
        .bmAttributes.usage_type = USB_USAGE_TYPE_DATA,
        .wMaxPacketSize = USB_CDC_NOTIFICATION_EP_SIZE,
        .bInterval = 8
    },
    // Data Class Interface
    .cdc_data_interface_2 = {
        .bLength = sizeof(struct usb_interface_descriptor),
        .bDescriptorType = USB_DESC_TYPE_INTERFACE,
        .bInterfaceNumber = USB_INTERFACE_CDC_DATA_2,
        .bAlternateSetting = 0,
        .bNumEndpoints = 2,
        .bInterfaceClass = USB_CDC_DATA_INTERFACE_CLASS,
        .bInterfaceSubClass = 0,
        .bInterfaceProtocol = 0,
        .iInterface = 0
    },
    .cdc_data_in_endpoint_2 = {
        .bLength = sizeof(struct usb_endpoint_descriptor),
        .bDescriptorType = USB_DESC_TYPE_ENDPOINT,
        .bEndpointAddress.direction = USB_DATA_TRANS_DEVICE_TO_HOST,
        .bEndpointAddress.endpoint_number = USB_CDC_DATA_IN_ENDPOINT_2,
        .bmAttributes.transfer_type = USB_TRANS_TYPE_BULK,
        .bmAttributes.sync_type = USB_SYNC_TYPE_NONE,
        .bmAttributes.usage_type = USB_USAGE_TYPE_DATA,
        .wMaxPacketSize = USB_CDC_DATA_EP_SIZE,
        .bInterval = 0
    },
    .cdc_data_out_endpoint_2 = {
        .bLength = sizeof(struct usb_endpoint_descriptor),
        .bDescriptorType = USB_DESC_TYPE_ENDPOINT,
        .bEndpointAddress.direction = USB_DATA_TRANS_HOST_TO_DEVICE,
        .bEndpointAddress.endpoint_number = USB_CDC_DATA_OUT_ENDPOINT_2,
        .bmAttributes.transfer_type = USB_TRANS_TYPE_BULK,
        .bmAttributes.sync_type = USB_SYNC_TYPE_NONE,
        .bmAttributes.usage_type = USB_USAGE_TYPE_DATA,
        .wMaxPacketSize = USB_CDC_DATA_EP_SIZE,
        .bInterval = 0
    }
#endif
#ifdef ENABLE_USB_STORAGE
    .association_storage = {
        .bLength = sizeof(struct usb_interface_association_descriptor),
        .bDescriptorType = USB_DESC_TYPE_INTERFACE_ASSOCIATION,
        .bFirstInterface = USB_INTERFACE_STORAGE,
        .bInterfaceCount = 1,
        .bFunctionClass = USB_STORAGE_CLASS_CODE,
        .bFunctionSubClass = USB_STORAGE_SUBCLASS_TRANSPARENT,
        .bFunctionProtocol = USB_STORAGE_PROTOCOL_CODE,
        .iFunction = 0
    },
    .storage_interface = {
        .bLength = sizeof(struct usb_interface_descriptor),
        .bDescriptorType = USB_DESC_TYPE_INTERFACE,
        .bInterfaceNumber = USB_INTERFACE_STORAGE,
        .bAlternateSetting = 0,
        .bNumEndpoints = 2,
        .bInterfaceClass = USB_STORAGE_CLASS_CODE,
        .bInterfaceSubClass = USB_STORAGE_SUBCLASS_TRANSPARENT,
        .bInterfaceProtocol = USB_STORAGE_PROTOCOL_CODE,
        .iInterface = 0
    },
    .storage_in_endpoint = {
        .bLength = sizeof(struct usb_endpoint_descriptor),
        .bDescriptorType = USB_DESC_TYPE_ENDPOINT,
        .bEndpointAddress.direction = USB_DATA_TRANS_DEVICE_TO_HOST,
        .bEndpointAddress.endpoint_number = USB_ENDPOINT_IN_STORAGE,
        .bmAttributes.transfer_type = USB_TRANS_TYPE_BULK,
        .bmAttributes.sync_type = USB_SYNC_TYPE_NONE,
        .bmAttributes.usage_type = USB_USAGE_TYPE_DATA,
        .wMaxPacketSize = USB_STORAGE_MAX_IN_BUFFER,
        .bInterval = 0
    },
    .storage_out_endpoint = {
        .bLength = sizeof(struct usb_endpoint_descriptor),
        .bDescriptorType = USB_DESC_TYPE_ENDPOINT,
        .bEndpointAddress.direction = USB_DATA_TRANS_HOST_TO_DEVICE,
        .bEndpointAddress.endpoint_number = USB_ENDPOINT_OUT_STORAGE,
        .bmAttributes.transfer_type = USB_TRANS_TYPE_BULK,
        .bmAttributes.sync_type = USB_SYNC_TYPE_NONE,
        .bmAttributes.usage_type = USB_USAGE_TYPE_DATA,
        .wMaxPacketSize = USB_STORAGE_MAX_OUT_BUFFER,
        .bInterval = 0
    }
#endif
};
