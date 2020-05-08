/**
 * @file scsi-standard.h
 * @desc Definitions relating to SCSI commands
 * @author Quinn Parrott
 * @date 2020-05-07
 */

#ifndef scsi_standard_h
#define scsi_standard_h

// Ignore warnings in this file about inefficient alignment
#pragma GCC diagnostic ignored "-Wattributes"
#pragma GCC diagnostic ignored "-Wpacked"

#include "global.h"

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
 * SCSI request sense reply
 *
 * Chapter 2 Page 65 of Oracle SCSI Reference Guide
 * @see https://docs.oracle.com/en/storage/tape-storage/storagetek-sl150-modular-tape-library/slorm/scsi-reference-guide.pdf
 */
struct scsi_request_sense_reply
{
    uint8_t responseCode : 7;
    uint16_t reserved1 : 9;
    uint8_t senseKey : 4;
    uint64_t reserved2 : 36;
    uint8_t additionalSenseLength : 8;
    uint32_t commandSpecificInformation : 32;
    uint8_t additionalSenseCode : 8;
    uint8_t additionalSenseCodeQualifier : 8;
    uint32_t reserved3 : 32;
} __attribute__((packed));

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
 * Section 5.3.{2, 9, 18} of Seagate SCSI Commands Reference Manual
 */
struct scsi_mode_sense_reply
{
    /* Header */
    uint8_t modeDataLength : 8;
    uint8_t mediumType : 8;
    uint8_t reserved1 : 7;
    uint8_t writeProtected : 1;
    uint8_t blockDescriptorLength : 8;

    /* Control Mode Page */
    uint8_t controlPageCode : 6;
    uint8_t controlSPF : 1;
    uint8_t controlPS : 1;
    uint8_t controlPageLength : 8;
    uint8_t RLEC : 1;
    uint8_t GLTSD : 1;
    uint8_t D_SENSE : 1;
    uint8_t DPICZ : 1;
    uint8_t TMF_ONLY : 1;
    uint8_t TST : 3;
    uint8_t DQUE_obsolete : 1;
    uint8_t QERR : 2;
    uint8_t NUAR : 1;
    uint8_t QueueAlgorithmModifier : 4;
    uint8_t EAERP_obsolete : 1;
    uint8_t UAAERP_obsolete : 1;
    uint8_t RAERP_obsolete : 1;
    uint8_t SWP : 1;
    uint8_t UA_INTLCK_CTRL : 2;
    uint8_t RAC : 1;
    uint8_t VS : 1;
    uint8_t AutoloadMode : 3;
    uint8_t reserved2 : 1;
    uint8_t RWWP : 1;
    uint8_t ATMPE : 1;
    uint8_t TAS : 1;
    uint8_t ATO : 1;
    uint16_t obsolete1;
    uint16_t controlBusyTimeoutPeriod;
    uint16_t controlExtendedSelfTestCompletionTime;

    /* Cache Mode Page */
    uint8_t cachePageCode : 6;
    uint8_t cacheSPF : 1;
    uint8_t cachePS : 1;
    uint8_t cachePageLength : 8;
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
    uint8_t reserved3;
    uint32_t obsolete2 : 24;

    /* Informational Exceptions Control Mode Page */
    uint8_t exceptPageCode : 6;
    uint8_t exceptSPF : 1;
    uint8_t exceptPS : 1;
    uint8_t exceptPageLength : 8;
    uint8_t options3;
    uint8_t MRIE : 4;
    uint8_t reserved4 : 4;
    uint32_t intervalTime;
    uint32_t reportCount;
} __attribute__((packed));

// Stop ignoring warnings about inefficient alignment
#pragma GCC diagnostic pop

#endif /* scsi_standard_h */
