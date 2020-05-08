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
#define USB_STORAGE_SIGNATURE 0x43425355

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
 * SCSI Opcodes
 *
 * SCSI Commands Reference Manual by Seagate
 * @see https://www.seagate.com/files/staticfiles/support/docs/manual/Interface%20manuals/100293068j.pdf
 */
enum scsi_opcodes
{
    SCSI_OPCODE_FORMAT_UNIT = 0x04,
    SCSI_OPCODE_INQUIRY = 0x12,
    SCSI_OPCODE_REQUEST_SENSE = 0x03,
    SCSI_OPCODE_READ_CAPACITY = 0x25,
    SCSI_OPCODE_READ_10 = 0x28,
    SCSI_OPCODE_WRITE_10 = 0x2a,
    SCSI_OPCODE_READ_16 = 0x88,
    SCSI_OPCODE_REPORT_LUNS = 0xa0,
    SCSI_OPCODE_SEND_DIAGNOSTIC = 0x1d,
    SCSI_OPCODE_TEST_UNIT_READY = 0x00,
    SCSI_MODE_SENSE_6 = 0x1a,
    SCSI_MEDIUM_REMOVAL = 0x1e,
    SCSI_SYNC_CACHE_10 = 0x35,
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