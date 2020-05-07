/**
 * @file scsi.c
 * @desc SCSI commands
 * @author Quinn Parrott
 * @date 2020-04-26
 * Last Author:
 * Last Edited On:
 */

#include "scsi.h"

#include <string.h>

#include "config.h"
#include "math_util.h"
#include "usb-storage-standard.h"

#include "fat.h"

static const uint64_t size_of_partition = 14336000;

static uint8_t usb_receive_next_command_callback(struct usb_storage_state *state)
{
    state->mode = NEXT_COMMAND;
    return 0;
}

static uint8_t usb_status_callback(struct usb_storage_state *state, uint8_t status)
{
    /* Clamp padding to not exceed buffer length */
    uint16_t padding_length = CLAMP_MAX(state->received_usb_command->dataTransferLength, USB_STORAGE_BLOCK_SIZE * USB_STORAGE_BLOCK_COUNT);

    /* Check if there is still padding left */
    if (state->received_usb_command->dataTransferLength > 0)
    {
        /* Clear buffer */
        memset(state->send_buffer, 0, padding_length);

        /* Added to residual byte count */
        state->residual_bytes += padding_length;

        /* Length of the padding to send */
        state->usb_packet_length = padding_length;

        /* No data is left, terminate packet */
        if (state->received_usb_command->dataTransferLength > 0)
        {
            state->mode = SEND_CONTINUE;
        }
        else
        {
            state->mode = SEND_DONE;
        }
    }
    else
    {
        /* No padding left, send status */
        struct usb_storage_command_status_wrapper *status_wrapper = (struct usb_storage_command_status_wrapper *)state->send_buffer;
        status_wrapper->signature = USB_STORAGE_SIGNATURE;
        status_wrapper->tag = state->received_usb_command->tag;
        status_wrapper->residue = state->residual_bytes;
        status_wrapper->status = status;

        state->usb_packet_length = sizeof(struct usb_storage_command_status_wrapper);

        /* Done SCSI transaction */
        state->mode = SEND_DONE;
        state->next_callback = usb_receive_next_command_callback;
    }

    // usb_status_callback cannot fail because it is the fallback for other commands failing
    return 0;
}

uint8_t usb_status_failed_callback(struct usb_storage_state *state)
{
    return usb_status_callback(state, 1);
}

uint8_t usb_status_success_callback(struct usb_storage_state *state)
{
    return usb_status_callback(state, 0);
}

uint8_t scsi_set_command_callback(struct usb_storage_state *state)
{
    /* Recast any sized command descriptor block into biggest for easier handling */

    // Since opcode is always in the same position any scsi command size can be used
    switch (state->received_scsi_command->_6.opcode)
    {
    case SCSI_OPCODE_FORMAT_UNIT:
        // Not implemented
        return 1;
        break;
    case SCSI_OPCODE_INQUIRY:
        state->next_callback = scsi_inquiry_callback;
        break;
    case SCSI_OPCODE_REQUEST_SENSE:
        // Not implemented
        return 1;
        break;
    case SCSI_OPCODE_READ_CAPACITY:
        state->next_callback = scsi_read_capacity_callback;
        break;
    case SCSI_OPCODE_READ_10:
        state->next_callback = scsi_read_10_callback;
        break;
    case SCSI_OPCODE_WRITE_10:
        // TODO: Needs to work
        break;
    case SCSI_OPCODE_READ_16:
        // Not implemented
        return 1;
        break;
    case SCSI_OPCODE_REPORT_LUNS:
        // Not implemented
        return 1;
        break;
    case SCSI_OPCODE_SEND_DIAGNOSTIC:
        // Not implemented
        return 1;
        break;
    case SCSI_OPCODE_TEST_UNIT_READY:
        /* Device is always ready */
        state->next_callback = usb_status_success_callback;
        break;
    case SCSI_MODE_SENSE_6:
        state->next_callback = scsi_mode_sense_callback;
        break;
    case SCSI_MEDIUM_REMOVAL:
        state->next_callback = usb_status_success_callback;
        break;
    case SCSI_SYNC_CACHE_10:
        state->next_callback = usb_status_success_callback;
        break;
    default:
        return 1;
    }
    return 0;
}

uint8_t scsi_inquiry_callback(struct usb_storage_state *state)
{
    struct scsi_inquiry_reply *inquiry_reply = (struct scsi_inquiry_reply *)state->send_buffer;
    // Direct Access Device
    inquiry_reply->peripheralDeviceType = 0;
    // Logical units
    inquiry_reply->peripheralQualifier = 0;
    // Removable?
    inquiry_reply->removableMedia = 1;
    // SCSI compliance (5 == SPC-3)
    inquiry_reply->version = 5;
    // 2 because all other versions are obsolete
    inquiry_reply->responseDataFormat = 2;
    // NormACA not supported
    inquiry_reply->NORMACA = 0;
    // HiSup not supported
    inquiry_reply->HISUP = 0;
    // Remaining length of the inquiry
    inquiry_reply->additionalLength = 31;
    // Stuff that's not supported
    inquiry_reply->SCCS = 0;
    inquiry_reply->ACC = 0;
    inquiry_reply->TPGS = 0;
    inquiry_reply->_3PC = 0;
    inquiry_reply->PROTECT = 0;
    inquiry_reply->ENCSERV = 0;
    inquiry_reply->MULTIP = 0;
    // Command Queuing: basic support
    inquiry_reply->BQUE = 1;
    inquiry_reply->CMDQUE = 0;
    // Zero all reserved and obsolete
    inquiry_reply->reserved1 = 0;
    inquiry_reply->reserved2 = 0;
    inquiry_reply->obsolete1 = 0;
    inquiry_reply->obsolete2 = 0;
    inquiry_reply->obsolete3 = 0;

    // Device info
    const char vendorID[10] = "CUInSpace";
    memcpy(inquiry_reply->vendorID, vendorID, sizeof(inquiry_reply->vendorID));
    const char productID[17] = "CarletonU Rocket";
    memcpy(inquiry_reply->productID, productID, sizeof(inquiry_reply->productID));
    const char productRevisionLevel[5] = "0001";
    memcpy(inquiry_reply->productRevisionLevel, productRevisionLevel, sizeof(inquiry_reply->productRevisionLevel));

    state->usb_packet_length = sizeof(struct scsi_inquiry_reply);
    state->next_callback = usb_status_success_callback;
    state->mode = SEND_DONE;
    return 0;
}

uint8_t scsi_read_capacity_callback(struct usb_storage_state *state)
{
    struct scsi_read_capacity_10_reply *read_capacity_reply = (struct scsi_read_capacity_10_reply *)state->send_buffer;
    /* These need to be big endian hence __REV */
    uint64_t fat_size = fat_get_total_sectors(size_of_partition);
    read_capacity_reply->logicalBlockAddress = __REV(fat_size);
    read_capacity_reply->blockLength = __REV(USB_STORAGE_BLOCK_SIZE);

    state->usb_packet_length = sizeof(struct scsi_read_capacity_10_reply);
    state->next_callback = usb_status_success_callback;
    state->mode = SEND_DONE;
    return 0;
}

uint8_t scsi_read_10_callback(struct usb_storage_state *state)
{
    /* Fill buffer with some numbers to distinguish it */
    uint8_t block_count = CLAMP_MAX(state->received_scsi_command->_10.length, USB_STORAGE_BLOCK_COUNT);
    for (uint8_t block_num = 0; block_num < block_count; block_num++)
    {
        uint8_t *offset_buffer = state->send_buffer + (USB_STORAGE_BLOCK_SIZE * block_num);
        uint64_t pos = state->received_scsi_command->_10.logicalBlockAddress;
        uint64_t sector = fat_translate_sector(pos, size_of_partition, offset_buffer);
        if (sector != ~((uint64_t)0))
        {
            // Is part of the file sector is offset
            // TODO: Read meaningful data instead of numbers
            char ch = 0;
            memset(offset_buffer, ch, 512);
            memcpy(offset_buffer, &sector, sizeof(uint64_t));
        }
        /* Subtract from the number of blocks left */
        state->received_scsi_command->_10.length -= 1;

        /* Point to the next block */
        state->received_scsi_command->_10.logicalBlockAddress += 1;
    }

    /* Transfer length */
    state->usb_packet_length = USB_STORAGE_BLOCK_SIZE * block_count;

    if (state->received_scsi_command->_10.length == 0)
    {
        /* No blocks left, send success */
        state->next_callback = usb_status_success_callback;
        state->mode = SEND_DONE;
    }
    else
    {
        /* This is still one packet since not all data has been transferred */
        state->mode = SEND_CONTINUE;
    }

    return 0;
}

uint8_t scsi_mode_sense_callback(struct usb_storage_state *state)
{
    struct scsi_mode_sense_reply *mode_sense_reply = (struct scsi_mode_sense_reply *)state->send_buffer;

    /* Size excluding itself */
    mode_sense_reply->modeDataLength = sizeof(struct scsi_mode_sense_reply) - 1;

    /* Header */
    mode_sense_reply->mediumType = 0;
    mode_sense_reply->reserved1 = 0;
    mode_sense_reply->writeProtected = 1;
    mode_sense_reply->blockDescriptorLength = 0;

    /* Control Mode Page */
    mode_sense_reply->controlPageCode = 0x0a;
    mode_sense_reply->controlSPF = 0;
    mode_sense_reply->controlPS = 0;
    mode_sense_reply->controlPageLength = 10;
    mode_sense_reply->RLEC = 0;
    mode_sense_reply->GLTSD = 0;
    mode_sense_reply->D_SENSE = 0;
    mode_sense_reply->DPICZ = 0;
    mode_sense_reply->TMF_ONLY = 0;
    mode_sense_reply->TST = 0;
    mode_sense_reply->DQUE_obsolete = 0;
    mode_sense_reply->QERR = 0b11;
    mode_sense_reply->NUAR = 0;
    mode_sense_reply->QueueAlgorithmModifier = 0;
    mode_sense_reply->EAERP_obsolete = 0;
    mode_sense_reply->UAAERP_obsolete = 0;
    mode_sense_reply->RAERP_obsolete = 0;
    /* Software Write Protect */
    mode_sense_reply->SWP = 1;
    mode_sense_reply->UA_INTLCK_CTRL = 0;
    mode_sense_reply->RAC = 0;
    mode_sense_reply->VS = 0;
    mode_sense_reply->AutoloadMode = 0;
    mode_sense_reply->reserved2 = 0;
    mode_sense_reply->RWWP = 0;
    mode_sense_reply->ATMPE = 0;
    mode_sense_reply->TAS = 0;
    mode_sense_reply->ATO = 1;
    mode_sense_reply->obsolete1 = 0;
    mode_sense_reply->controlBusyTimeoutPeriod = 10;
    mode_sense_reply->controlExtendedSelfTestCompletionTime = 0;

    /* Cache Mode Page */
    mode_sense_reply->cachePageCode = 0x8;
    mode_sense_reply->cachePS = 0;
    mode_sense_reply->cacheSPF = 0;
    mode_sense_reply->cachePageLength = 18;
    mode_sense_reply->options1 = 0b100;
    mode_sense_reply->writeRetentionPriority = 0;
    mode_sense_reply->readRetentionPriority = 0;
    mode_sense_reply->disablePrefetchExceeds = 0;
    mode_sense_reply->minimumPrefetch = 0;
    mode_sense_reply->maximumPrefetch = 0;
    mode_sense_reply->maximumPrefetchCeiling = 0;
    mode_sense_reply->options2 = 0;
    mode_sense_reply->numberCache = 0;
    mode_sense_reply->cacheSegmentSize = 0;
    mode_sense_reply->reserved3 = 0;
    mode_sense_reply->obsolete2 = 0;

    /* Informational Exceptions Control Mode Page */
    mode_sense_reply->exceptPageCode = 0x1c;
    mode_sense_reply->exceptSPF = 0;
    mode_sense_reply->exceptPS = 0;
    mode_sense_reply->exceptPageLength = 10;
    mode_sense_reply->options3 = 0;
    mode_sense_reply->MRIE = 0;
    mode_sense_reply->reserved4 = 0;
    mode_sense_reply->intervalTime = 0;
    mode_sense_reply->reportCount = 0;

    state->usb_packet_length = state->received_usb_command->dataTransferLength;
    state->next_callback = usb_status_success_callback;
    state->mode = SEND_DONE;
    return 0;
}
