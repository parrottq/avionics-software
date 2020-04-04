/**
 * @file fat.h
 * @desc Format memory chucks with FAT32
 * @author Quinn Parrott
 * @date 2020-03-04
 * Last Author:
 * Last Edited On:
 */

#ifndef fat_h
#define fat_h

#include <stdint.h>

#include "fat-standard.h"

#define FAT_SECTOR_SIZE 512
#define FAT_CLUSTER_SIZE 128

/**
 * 1. Directory builder
 * 2. Files
 */

/**
 * 1. Boot sector
 * -root clust
 * -fsinfo
 * -total sect
 * 2. fsinfo
 * 3. Fat
 * 4. Data
 */

/*
Background

There are 3 regions to a fat32 partition that in order are:
1. Reserved Region
Reserved contains the boot sector and fsinfo. If desired some
boot code could be inserted here as it is of variable size.
2. File Allocation Region (FAT)
List of all clusters sizes and file order
3. Data Region
Contains directory data and file contents
*/

// TODO: Update
/**
 * Translates a block number into the current file and it's block number.
 * 
 * @param block Block number that will be translated
 * @param size the size in bytes of the file
 * @param buffer 512 byte buffer that is written to if the block is filesystem related
 * 
 * @note If the file.id is 0xffff then this is not a file block and unless the function failed,
 * filesystem data has been written to the buffer. 
 * 
 * @return != 0 if failure
 */
extern uint64_t fat_translate_sector(uint64_t block, uint64_t size, uint8_t *buffer);

#endif /* fat_h */
