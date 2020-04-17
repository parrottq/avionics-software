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

/*
Terminology

FAT:
File Allocation Table. Describes the allocation of all clusters. A
single part of the FAT32 partition.

FAT32:
The partition type fat.h and fat.c implement. Not to be confused
with the FAT (File Allocation Table) that is only a component of a
FAT32 partition. When 'FAT' is written without 'partition' or a
number suffixed the File Allocation Table is what is being referenced.

Sector:
Continuous 512 byte data chunk that the whole partition is divided into.

Cluster:
Discontinuous collection of sectors linked together by the FAT. Clusters
can only be found after the FAT on the partition.

Cluster Chain:
A single linked list found in the FAT linking clusters together into a file
or directory.


FAT32's file structure
(In sequential order)

Boot sector:
First sector containing general information about the partition.

File System Info (FSInfo) sectors:
Usually the second sector. Contains the free cluster count and
the next free cluster. Should not be taken as accurate.

File Allocation Table (FAT) sectors:
The FAT contains back-to-back uint32_t(s) for every cluster. It describes
if the cluster is allocated, reserved, pointing to the next cluster in a
chain or is the end of a cluster chain. Individual uint32_t entries come
together to make a cluster chain that describes the location of the contents
of a file or directory in the partition. The first two entries of the FAT are
reserved. The FAT's size is given by the boot sector.

Directory cluster:
A directory cluster contains information and links to it's sub-files
and sub-directories. Eg. Filename, First cluster a file, File size, etc.
The root directory is given in the Boot sector.

File cluster:
The contents of the files as allocated in the FAT much like the directory cluster.
*/

#include "fat-standard.h"

/**
 * The size of a fat sector. In theory this value
 * can be changed but is always 512.
 */
#define FAT_SECTOR_SIZE 512

/**
 * The number of clusters per sector. Can be changed
 * to any power of two between 1 <= x <= 128.
 */
#define FAT_SECTOR_PER_CLUSTER 4

/**
 * Calculate the number of sectors that are needed to represent a number
 * of bytes as a FAT32 partition.
 * 
 * @param size Total size in bytes of the data that the user wishes to represent
 * as a FAT32 partition.
 * 
 * @return Number of sectors that are needed to represent the data.
 * @note See FAT_SECTOR_SIZE for the size of a sector.
 */
extern uint32_t fat_get_total_sectors(uint64_t size);

/**
 * Translates a sector number into the data's block number.
 * 
 * This is the main formatter. When executed the formater will decide if the current sector
 * is part of the file system (that it itself must populate) or part of the data (that the
 * caller must populate). If the information is file system related, said information is put
 * into 'buffer' and the function returns a full uint64_t (see return). If the information
 * is user data, the offset is sent back and, presumably, the user fills the buffer with their data.
 * 
 * @param sector Position of the sector currently being requested.
 * @param size Total size in bytes of the data that the user wishes to represent
 * as a FAT32 partition. Should not change or other sector positions will change aswell.
 * @param buffer 512 byte buffer that should be written to if the block is filesystem related.
 * 
 * @return If a full uint64_t ~((uint64_t) 0) read buffer, else the offset of the block of data
 * to write to this sector is returned.
 */
extern uint64_t fat_translate_sector(uint64_t sector, uint64_t size, uint8_t buffer[]);

/* Internal format functions */

/**
 * Format the boot sector of a FAT32 partition
 * 
 * @param buffer 512 byte buffer to write the data to.
 * @param data_size_bytes Total size in bytes of the data that the user wishes to represent.
 * See fat_translate_sector:size
 */
extern void fat_format_boot(uint8_t buffer[], uint64_t data_size_byte);

/**
 * Format the FSInfo sector of a FAT32 partition
 * 
 * @param buffer 512 byte buffer to write the data to.
 */
extern void fat_format_fsinfo(uint8_t buffer[]);

/**
 * Format a single FAT sector
 * 
 * @param buffer 512 byte buffer to write the data to.
 * @param current_block Index of the relative FAT sector, not the absolute sector
 * @param data_size_bytes Total size in bytes of the data that the user wishes to represent.
 * See fat_translate_sector:size
 */
extern void fat_format_fat(uint8_t buffer[], uint64_t current_block, uint64_t data_size_byte);

/**
 * Format a single directory sector
 * 
 * @param buffer 512 byte buffer to write the data to.
 * @param current_block Index of the relative FAT sector, not the absolute sector
 * @param data_size_bytes Total size in bytes of the data that the user wishes to represent.
 * See fat_translate_sector:size
 */
extern void fat_format_dir(uint8_t buffer[], uint64_t current_block, uint64_t data_size_byte);

#endif /* fat_h */
