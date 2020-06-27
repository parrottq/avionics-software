/**
 * @file usb-storage-standard.h
 * @desc Definitions from relating to USB Mass Storage devices
 * @author Quinn Parrott
 * @date 2020-02-13
 */

#ifndef usb_storage_standard_h
#define usb_storage_standard_h

// Ignore warnings in this file about inefficient alignment
#pragma GCC diagnostic ignored "-Wattributes"
#pragma GCC diagnostic ignored "-Wpacked"

#define USB_STORAGE_CLASS_CODE 0x08

/* SCSI instuction set not reported */
#define USB_STORAGE_SUBCLASS_NONE 0x00
/* SCSI Reduced Block Command instruction set */
#define USB_STORAGE_SUBCLASS_RBC 0x01
/* SCSI transparent command set */
#define USB_STORAGE_SUBCLASS_TRANSPARENT 0x06

/* USB Mass Storage Class Bulk-Only Transport */
#define USB_STORAGE_PROTOCOL_CODE 0x50

/* USB Mass Storage Command Block Wrapper Signature */
#define USB_STORAGE_COMMAND_BLOCK_WRAPPER_SIGNATURE  0x43425355
/* USB Mass Storage Command Status Wrapper Signature */
#define USB_STORAGE_COMMAND_STATUS_WRAPPER_SIGNATURE 0x53425355

/* Common block size */
#define USB_STORAGE_BLOCK_SIZE 512

/**
 * USB Mass Storage class specific as defined in
 *
 * USB Mass Storage Class Bulk-Only Transport Rev 1.0 Section 3
 * @see https://www.usb.org/sites/default/files/usbmassbulk_10.pdf
 */
enum usb_storage_class_specific
{
    USB_STORAGE_REQ_RESET = 0xff,
    USB_STORAGE_REQ_MAX_LUN = 0xfe,
};

/**
 * USB Mass Storage Command Block Wrapper and
 * a SCSI Command Desctiptor Block.
 *
 * Section 5.1 for Command Block Wrapper
 * @see https://www.usb.org/sites/default/files/usbmassbulk_10.pdf
 */
struct usb_storage_command_block_wrapper
{
    /* This will always be 'USBC' */
    uint32_t signature;
    /* Transaction number */
    uint32_t tag;
    /* Expected length of next command */
    uint32_t dataTransferLength;
    /* 0x80 if response data is expected */
    uint8_t flags;
    /* Logical Unit Number */
    uint8_t lun;
    /* Command block length */
    uint8_t SCSILength;
} __attribute__((packed));

/**
 * USB Mass Storage Command Status Wrapper
 *
 * Section 5.2
 * @see https://www.usb.org/sites/default/files/usbmassbulk_10.pdf
 */
struct usb_storage_command_status_wrapper
{
    /* This will always be 'USBC' */
    uint32_t signature;
    /* Transaction number */
    uint32_t tag;
    /* Amount of data not processed */
    uint32_t residue;
    /* Status */
    uint8_t status;
} __attribute__((packed));

// Stop ignoring warnings about inefficient alignment
#pragma GCC diagnostic pop

#endif /* usb_storage_standard_h */