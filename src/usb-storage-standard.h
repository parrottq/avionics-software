/**
 * @file usb-storage-standard.h
 * @desc Definitions from relating to USB Mass Storage devices
 * @author Quinn Parrott
 * @date 2020-02-13
 * Last Author:
 * Last Edited On:
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

/**
 * SCSI Command Descriptor Blocks (6 bytes)
 *
 * Section 2.1.2 for Command Descriptor Block
 * @see https://www.seagate.com/files/staticfiles/support/docs/manual/Interface%20manuals/100293068j.pdf
 */
struct scsi_command_descriptor_block_6
{
    /* Opcode */
    uint8_t opcode;
    /* Additional options */
    uint8_t options;
    /* Requested block */
    uint16_t logicalBlockAddress;
    /* Generic length, changes meaning with different commands */
    uint8_t length;
    /* Controls a variety of options */
    uint8_t control;
} __attribute__((packed)) scsi_command_descriptor_block_6;

/**
 * SCSI Command Descriptor Blocks (10 bytes)
 *
 * Section 2.1.2 for Command Descriptor Block
 * @see https://www.seagate.com/files/staticfiles/support/docs/manual/Interface%20manuals/100293068j.pdf
 */
struct scsi_command_descriptor_block_10
{
    /* Opcode */
    uint8_t opcode;
    /* Additional options */
    uint8_t options;
    /* Requested block */
    uint32_t logicalBlockAddress;
    /* Even more options */
    uint8_t options2;
    /* Generic length, changes meaning with different commands */
    uint16_t length;
    /* Controls a variety of options */
    uint8_t control;
} __attribute__((packed)) scsi_command_descriptor_block_10;

/**
 * SCSI Command Descriptor Blocks (12 bytes)
 *
 * Section 2.1.2 for Command Descriptor Block
 * @see https://www.seagate.com/files/staticfiles/support/docs/manual/Interface%20manuals/100293068j.pdf
 */
struct scsi_command_descriptor_block_12
{
    /* Opcode */
    uint8_t opcode;
    /* Additional options */
    uint8_t options;
    /* Requested block */
    uint32_t logicalBlockAddress;
    /* Generic length, changes meaning with different commands */
    uint32_t length;
    /* More options */
    uint8_t options2;
    /* Controls a variety of options */
    uint8_t control;
} __attribute__((packed)) scsi_command_descriptor_block_12;

/**
 * SCSI Command Descriptor Blocks (16 bytes)
 *
 * Section 2.1.2 for Command Descriptor Block
 * @see https://www.seagate.com/files/staticfiles/support/docs/manual/Interface%20manuals/100293068j.pdf
 */
struct scsi_command_descriptor_block_16
{
    /* Opcode */
    uint8_t opcode;
    /* Additional options */
    uint8_t options;
    /* Requested block */
    uint64_t logicalBlockAddress;
    /* Generic length, changes meaning with different commands */
    uint32_t length;
    /* More options */
    uint8_t options2;
    /* Controls a variety of options */
    uint8_t control;
} __attribute__((packed)) scsi_command_descriptor_block_16;

/**
 * SCSI Command Descriptor Block with all sizes accessible through a union
 */
union scsi_command_descriptor_block
{
    struct scsi_command_descriptor_block_6 _6;
    struct scsi_command_descriptor_block_10 _10;
    struct scsi_command_descriptor_block_12 _12;
    struct scsi_command_descriptor_block_16 _16;
} __attribute__((packed)) scsi_command_descriptor_block;

/**
 * SCSI Read Capacity reply
 *
 * Note: These values are big endian for some reason
 *
 * Section 3.22.2 of Seagate SCSI Commands Referenc Manual
 */
struct scsi_read_capacity_10_reply
{
    /* Logical Block Address */
    uint32_t logicalBlockAddress;
    /* Block Length */
    uint32_t blockLength;
} __attribute__((packed));

/**
 * SCSI Inquiry reply
 *
 * Section 3.6.2 of Seagate SCSI Commands Referenc Manual
 */
struct scsi_inquiry_reply
{
    uint8_t peripheralDeviceType : 5;
    uint8_t peripheralQualifier : 3;
    uint8_t reserved1 : 7;
    uint8_t removableMedia : 1;
    uint8_t version : 8;
    uint8_t responseDataFormat : 4;
    uint8_t HISUP : 1;
    uint8_t NORMACA : 1;
    uint8_t obsolete1 : 2;
    uint8_t additionalLength : 8;
    uint8_t PROTECT : 1;
    uint8_t reserved2 : 2;
    uint8_t _3PC : 1;
    uint8_t TPGS : 2;
    uint8_t ACC : 1;
    uint8_t SCCS : 1;
    uint8_t obsolete2 : 4;
    uint8_t MULTIP : 1;
    uint8_t VS1 : 1;
    uint8_t ENCSERV : 1;
    uint8_t BQUE : 1;
    uint8_t VS2 : 1;
    uint8_t CMDQUE : 1;
    uint8_t obsolete3 : 6;
    char vendorID[8];
    char productID[16];
    char productRevisionLevel[4];
} __attribute__((packed));

/**
 * SCSI mode sense reply
 *
 * Section 5.3.{2, 9, 18} of Seagate SCSI Commands Referenc Manual
 */
struct scsi_mode_sense_reply
{
    /* Header */
    uint8_t modeDataLength : 8;
    uint8_t mediumType : 8;
    uint8_t reserved1 : 8;
    uint8_t blockDescriptorLength : 8;

    /* Cache Mode Page */
    uint8_t cachePageCode : 6;
    uint8_t cacheSPF : 1;
    uint8_t cachePS : 1;
    uint8_t cachePageLength;
    uint8_t options1;
    uint8_t writeRetentionPriority : 4;
    uint8_t readRetentionPriority : 4;
    uint16_t disablePrefetchExceeds;
    uint16_t minimumPrefetch;
    uint16_t maximumPrefetch;
    uint16_t maximumPrefetchCeiling;
    uint8_t options2;
    uint8_t numberCache;
    uint16_t cacheSegmentSize;
    uint8_t reserved2;
    uint32_t obsolete1 : 24;

    /* Informational Exceptions Control Mode Page */
    uint8_t exceptPageCode : 6;
    uint8_t exceptSPF : 1;
    uint8_t exceptPS : 1;
    uint8_t exceptPageLength;
    uint8_t options3;
    uint8_t MRIE : 4;
    uint8_t reserved3 : 4;
    uint32_t intervalTime;
    uint32_t reportCount;
} __attribute__((packed));

// Stop ignoring warnings about inefficient alignment
#pragma GCC diagnostic pop

#endif /* usb_storage_standard_h */