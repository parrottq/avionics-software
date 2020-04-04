/**
 * @file fat-standard.h
 * @desc Definitions from relating FAT32 partitioning
 * @author Quinn Parrott
 * @date 2020-03-04
 * Last Author:
 * Last Edited On:
 */

#ifndef fat_standard_h
#define fat_standard_h

#include <stdint.h>

/**
 * Represents the minimum number of reserved sectors
 * needed. Boot (1) + FSInfo (1) = 2
 */
#define FAT_RESERVED_SECTORS 2

/**
 * Cluster offset is the number of entries in the FAT
 * that are reserved.
 */
#define FAT_CLUSTER_OFFSET 2

/**
 * Size of a directory entry in bytes.
 */
#define FAT_DIR_ENTRY_SIZE 32

/**
 * Size of a FAT entry.
 */
#define FAT_ENTRY_SIZE sizeof(uint32_t)

// Ignore warnings in this file about inefficient alignment
#pragma GCC diagnostic ignored "-Wattributes"
#pragma GCC diagnostic ignored "-Wpacked"

/**
 * The first sector of a fat partition
 * 
 * Note signature needs to be set at offset 510,
 * the signature consists of 0x55AA (2 bytes)
 */
struct fat_boot_sector_head
{
    /* FAT 12, 16 and 32 boot section */

    char BS_jmpBoot[3];
    uint64_t BS_OEMName;
    uint16_t BPB_BytsPerSec;
    uint8_t BPB_SecPerClus;
    /* Reserved chucks (0<) */
    uint16_t BPB_RsvdSecCnt;
    /* Number of fats (usually 2) */
    uint8_t BPB_NumFATs;
    uint16_t BPB_RootEntCnt; // FAT32: 0
    uint16_t BPB_TotSec16;   // FAT32: 0
    uint8_t BPB_Media;
    uint16_t BPB_FATSz16; // FAT32: 0
    uint16_t BPB_SecPerTrk;
    uint16_t BPB_NumHeads;
    uint32_t BPB_HiddSec; // 0 if not partitioned
    /* The total number of sectors */
    uint32_t BPB_TotSec32;

    /* FAT32 specific boot section */

    /* Number of sectors occupied by one fat */
    uint32_t BPB_FATSz32;
    uint16_t BPB_ExtFlags;
    uint16_t BPB_FSVer;
    uint32_t BPB_RootClus;
    uint16_t BPB_FSInfo;
    uint16_t BPB_BkBootSec;
    char BPB_Reserved[12];
    uint8_t BS_DrvNum;
    uint8_t BS_Reserved1;
    uint8_t BS_BootSig;
    uint32_t BS_VolID;
    char BS_VolLab[11];
    uint64_t BS_FilSysType;
} __attribute__((packed));

/**
 * File System Information (FSInfo) Structure tail
 * Note starts at offset 484 in the sector
 * 
 * Also needed is a lead signature of 0x41615252 at offset 0
 * and trailing signature of 0xAA550000 at 508
 * 
 * See the spec for more details
 */
struct fat_file_system_info
{
    uint32_t FSI_StrucSig;
    uint32_t FSI_Free_Count;
    uint32_t FSI_Nxt_Free;
} __attribute__((packed));

/**
 * Directory Structure
 */
struct fat_directory
{
    char DIR_Name[11];
    uint8_t DIR_Attr;
    uint8_t DIR_NTRes;
    uint8_t DIR_CrtTimeTenth;
    uint16_t DIR_CrtTime;
    uint16_t DIR_CrtDate;
    uint16_t DIR_LstAccDate;
    uint16_t DIR_FstClusHI;
    uint16_t DIR_WrtTime;
    uint16_t DIR_WrtDate;
    uint16_t DIR_FstClusLO;
    uint32_t DIR_FileSize;
} __attribute__((packed));

// Stop ignoring warnings about inefficient alignment
#pragma GCC diagnostic pop

#endif /* fat_standard_h */