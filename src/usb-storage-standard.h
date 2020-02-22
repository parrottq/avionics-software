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

#define USB_STORAGE_CLASS_CODE 0x08

/* SCSI instuction set not reported */
#define USB_STORAGE_SUBCLASS_NONE 0x00
/* SCSI Reduced Block Command instruction set */
#define USB_STORAGE_SUBCLASS_RBC 0x01
/* SCSI transparent command set */
#define USB_STORAGE_SUBCLASS_TRANSPARENT 0x06

/* USB Mass Storage Class Bulk-Only Transport */
#define USB_STORAGE_PROTOCOL_CODE 0x50

#endif /* usb_storage_standard_h */