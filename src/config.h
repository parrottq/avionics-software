/**
 * @file config.h
 * @desc Hardware Configuration
 * @author Samuel Dewan
 * @date 2019-02-15
 * Last Author:
 * Last Edited On:
 */

#ifndef config_h
#define config_h

/* MCU Board Pinout Configuration */
#include "pins-rev-a.h"
//#include "pins-rev-b.h"

/* Peripheral/Software configuration= */
#include "config-test.h"
//#include "config-rocket.h"
//#include "config-ground.h"


/* USB Strings */
#define USB_MANUFACTURER_STRING u"CU InSpace"
#define USB_PRODUCT_STRING u"CU InSpace MCU Board"

// The make file will make this the current git hash of HEAD
#ifndef AVIONICS_VERSION
#define AVIONICS_VERSION u"00001"
#endif

#endif /* config_h */
