/**
 * @file fat.c
 * @desc Format memory chucks with FAT32
 * @author Quinn Parrott
 * @date 2020-03-04
 * Last Author:
 * Last Edited On:
 */

#include "fat.h"

#include <string.h>
#include <stdio.h>

// The brackets around each variable are necessary so inline math works correctly
#define INTEGER_DIVISION_ROUND_UP(dividend, divisor) (((dividend) / (divisor)) + (((dividend) % (divisor)) > 0))

// Rollover is the number of cluster per file
static const uint64_t rollover = (((uint64_t)1) << 32) / (FAT_SECTOR_SIZE * FAT_SECTOR_PER_CLUSTER) - 1;

static uint64_t fat_calc_file_size_cluster(uint64_t data_size_byte)
{
    // '+ 1' is a harmless change that stops warning from 'dosfsck'
    return INTEGER_DIVISION_ROUND_UP(data_size_byte, 512 * FAT_SECTOR_PER_CLUSTER) + 1;
}

static uint64_t fat_calc_dir_size_cluster(uint64_t data_size_byte)
{
    return INTEGER_DIVISION_ROUND_UP(data_size_byte, rollover * FAT_SECTOR_SIZE * FAT_SECTOR_PER_CLUSTER * (FAT_SECTOR_SIZE * FAT_SECTOR_PER_CLUSTER / FAT_DIR_ENTRY_SIZE));
}

static uint32_t fat_calc_fat_size_sector(uint64_t file_size_cluster, uint64_t dir_size_cluster)
{
    return INTEGER_DIVISION_ROUND_UP(dir_size_cluster + file_size_cluster, FAT_SECTOR_SIZE / FAT_ENTRY_SIZE);
}

uint32_t fat_get_total_sectors(uint64_t data_size_byte)
{
    uint64_t file_size_cluster = fat_calc_file_size_cluster(data_size_byte);
    uint64_t dir_size_cluster = fat_calc_dir_size_cluster(data_size_byte);
    uint32_t fat_size_sector = fat_calc_fat_size_sector(file_size_cluster, dir_size_cluster);
    return FAT_RESERVED_SECTORS + fat_size_sector + FAT_SECTOR_PER_CLUSTER * (FAT_CLUSTER_OFFSET + dir_size_cluster + file_size_cluster);
}

void fat_format_boot(uint8_t buffer[], uint64_t data_size_byte)
{
    /* Boot sector */
    printf("Boot block\n");

    uint64_t file_size_cluster = fat_calc_file_size_cluster(data_size_byte);
    uint64_t dir_size_cluster = fat_calc_dir_size_cluster(data_size_byte);
    uint32_t fat_size_sector = fat_calc_fat_size_sector(file_size_cluster, dir_size_cluster);

    struct fat_boot_sector_head *sector = (struct fat_boot_sector_head *)buffer;
    sector->BS_jmpBoot[0] = 0xeb;
    sector->BS_jmpBoot[1] = 0x58;
    sector->BS_jmpBoot[2] = 0x90;

    const char oem_name[] = "CuInSpac";
    memcpy(&sector->BS_OEMName, oem_name, sizeof(sector->BS_OEMName));

    sector->BPB_BytsPerSec = FAT_SECTOR_SIZE;
    sector->BPB_SecPerClus = FAT_SECTOR_PER_CLUSTER;
    sector->BPB_RsvdSecCnt = FAT_RESERVED_SECTORS;
    sector->BPB_NumFATs = 1;
    sector->BPB_RootEntCnt = 0;
    sector->BPB_TotSec16 = 0;
    sector->BPB_Media = 0xf8;
    sector->BPB_FATSz16 = 0;
    sector->BPB_SecPerTrk = 32;
    sector->BPB_NumHeads = 64;
    sector->BPB_HiddSec = 0;
    uint32_t total_sectors = FAT_RESERVED_SECTORS + fat_size_sector + (FAT_SECTOR_PER_CLUSTER * file_size_cluster);
    sector->BPB_TotSec32 = total_sectors;

    sector->BPB_FATSz32 = fat_size_sector;
    sector->BPB_ExtFlags = 0;
    sector->BPB_FSVer = 0;
    sector->BPB_RootClus = FAT_CLUSTER_OFFSET;
    sector->BPB_FSInfo = 1;
    sector->BPB_BkBootSec = 0;

    memset(sector->BPB_Reserved, 0, sizeof(sector->BPB_Reserved));

    sector->BS_DrvNum = 0;
    sector->BS_Reserved1 = 0;
    sector->BS_BootSig = 0x29;
    sector->BS_VolID = 0;
    const char fat_volume_label[] = "MCU Board  ";
    memcpy(sector->BS_VolLab, fat_volume_label, sizeof(sector->BS_VolLab));
    const char fat_fs_type[] = "FAT32      ";
    memcpy(&sector->BS_FilSysType, fat_fs_type, sizeof(sector->BS_FilSysType));

    memset(
        buffer + sizeof(struct fat_boot_sector_head), 0,
        512 - (2 + sizeof(struct fat_boot_sector_head)));

    buffer[510] = 0x55;
    buffer[511] = 0xAA;
}

void fat_format_fsinfo(uint8_t buffer[])
{
    /* FSinfo */
    printf("FSInfo\n");

    buffer[0] = 0x52;
    buffer[1] = 0x52;
    buffer[2] = 0x61;
    buffer[3] = 0x41;

    memset(buffer + 4, 0, 480);

    struct fat_file_system_info *sector = (struct fat_file_system_info *)(buffer + 484);
    sector->FSI_StrucSig = 0x61417272;
    // 0xF... means not calculated
    sector->FSI_Free_Count = 0xffffffff;
    sector->FSI_Nxt_Free = 0xffffffff;

    memset(buffer + 492, 0, 12);

    buffer[510] = 0x55;
    buffer[511] = 0xAA;
}

void fat_format_fat(uint8_t buffer[], uint64_t current_block, uint64_t data_size_byte)
{
    /* FAT sectors */
    printf("FAT\n");

    uint64_t file_size_cluster = fat_calc_file_size_cluster(data_size_byte);
    uint64_t dir_size_cluster = fat_calc_dir_size_cluster(data_size_byte);

    uint64_t fat_entry = 0;
    fat_entry += current_block * (FAT_SECTOR_SIZE / FAT_ENTRY_SIZE);
    for (uint32_t entry_offset = 0; entry_offset < FAT_SECTOR_SIZE; entry_offset += FAT_ENTRY_SIZE)
    {
        uint32_t entry = 0;

        if (fat_entry < FAT_CLUSTER_OFFSET)
        {
            printf("\tRes cluster: ");

            // First 2 entries are reserved
            if (fat_entry == 0)
            {
                entry = 0x0ffffff8;
            }
            else if (fat_entry == 1)
            {
                entry = 0x0fffffff;
            }
        }
        else if (fat_entry < (FAT_CLUSTER_OFFSET + dir_size_cluster))
        {
            printf("\tDIR entry\n");
            // Directory entry
            uint64_t fat_dir_entry = fat_entry - FAT_CLUSTER_OFFSET;
            if (fat_dir_entry == (dir_size_cluster - 1))
            {
                entry = 0x0fffffff;
            }
            else
            {
                entry = fat_entry + 1;
            }
        }
        else
        {
            printf("\tFile entry: ");
            // File entry
            uint64_t fat_file_entry = fat_entry - (dir_size_cluster + FAT_CLUSTER_OFFSET);
            printf("%llu\n", fat_file_entry);
            if (fat_file_entry >= file_size_cluster)
            {
                // Outside the data bound, mark unallocated
                entry = 0;
            }
            else if ((fat_file_entry % rollover) == (rollover - 1) || fat_file_entry == (file_size_cluster - 1))
            {
                // End of the file
                // Can be triggered by exceeding a file size (rollover) or the end of the data bound
                entry = 0x0fffffff;
            }
            else
            {
                // In data bound, point to next entry to make clusters continuous
                entry = fat_entry + 1;
            }
        }

        memcpy(buffer + entry_offset, &entry, sizeof(uint32_t));

        fat_entry++;
    }
}

inline static void format_dir_name(uint32_t entry, struct fat_directory *dir)
{
    // The first 8 bytes are for file name
    uint32_t div = 1;
    for (uint8_t i = 0; i < 8; i++)
    {
        dir->DIR_Name[7 - i] = ((entry / div) % 10) + 48;
        div *= 10;
    }

    // The last 3 bytes are for file extension
    for (uint8_t i = 8; i < 11; i++)
    {
        dir->DIR_Name[i] = ' ';
    }
}

void fat_format_dir(uint8_t buffer[], uint64_t current_block, uint64_t data_size_byte)
{
    /* DIR clusters */
    printf("DIR\n");

    uint64_t dir_size_cluster = fat_calc_dir_size_cluster(data_size_byte);

    const uint16_t dir_entries_per_sector = (FAT_SECTOR_SIZE / FAT_DIR_ENTRY_SIZE);
    uint32_t dir_entry = dir_entries_per_sector * current_block;
    for (uint16_t dir_entry_offset = 0; dir_entry_offset < FAT_SECTOR_SIZE; dir_entry_offset += sizeof(struct fat_directory))
    {
        struct fat_directory *entry = (struct fat_directory *)(buffer + dir_entry_offset);

        if (data_size_byte > (dir_entry * rollover * FAT_SECTOR_PER_CLUSTER * FAT_SECTOR_SIZE))
        {
            // Name of the file is just a number for easy sorting
            format_dir_name(dir_entry, entry);

            // Read-only flag
            entry->DIR_Attr = 1;
            entry->DIR_NTRes = 0;
            entry->DIR_CrtTimeTenth = 0;
            entry->DIR_CrtTime = 0;
            entry->DIR_CrtDate = 0;
            entry->DIR_LstAccDate = 0;
            entry->DIR_WrtTime = 0;
            entry->DIR_WrtDate = 0;

            // The location of the first cluster
            uint32_t start_cluster = dir_entry * rollover + FAT_CLUSTER_OFFSET + dir_size_cluster;
            entry->DIR_FstClusHI = (uint16_t)(start_cluster >> 16) & 0xffff;
            entry->DIR_FstClusLO = (uint16_t)start_cluster & 0xffff;

            // The size of the file
            uint64_t file_size_left_bytes = data_size_byte - (dir_entry * rollover * FAT_SECTOR_PER_CLUSTER * FAT_SECTOR_SIZE);
            if (file_size_left_bytes > (rollover * FAT_SECTOR_PER_CLUSTER * FAT_SECTOR_SIZE))
            {
                entry->DIR_FileSize = rollover * FAT_SECTOR_PER_CLUSTER * FAT_SECTOR_SIZE;
            }
            else
            {
                entry->DIR_FileSize = file_size_left_bytes;
            }
            printf("\tStart cluster: %lu file_size_left_bytes: %llu size: %lu\n", start_cluster, file_size_left_bytes, entry->DIR_FileSize);
        }
        else
        {
            memset(entry, 0, sizeof(struct fat_directory));
            printf("Empty dir entry\n");
        }
        // Next dir entry, keep in sync with loop
        dir_entry++;
    }
}

uint64_t fat_translate_sector(uint64_t block, uint64_t size, uint8_t buffer[])
{
    uint64_t data_size_byte = size;
    uint64_t file_size_cluster = fat_calc_file_size_cluster(size);
    uint64_t dir_size_cluster = fat_calc_dir_size_cluster(size);
    uint32_t fat_size_sector = fat_calc_fat_size_sector(file_size_cluster, dir_size_cluster);

    uint32_t offset_fat = FAT_RESERVED_SECTORS + fat_size_sector;
    uint64_t offset_dir = offset_fat + FAT_SECTOR_PER_CLUSTER * dir_size_cluster;
    uint64_t offset_file = offset_dir + FAT_SECTOR_PER_CLUSTER * file_size_cluster;

    const uint64_t BUFFER_CHANGED = ~((uint64_t)0);

    if (block == 0)
    {
        /* Boot sector */
        fat_format_boot(buffer, data_size_byte);
    }
    else if (block == 1)
    {
        /* File System Info (FSInfo) sector */
        fat_format_fsinfo(buffer);
    }
    else if (block < FAT_RESERVED_SECTORS)
    {
        // If there are any other reserved sectors, they will be empty
        printf("Reserved sector\n");
    }
    else if (block < offset_fat)
    {
        /* File Allocation Table (FAT) sectors */
        uint64_t current_block = block;
        current_block -= FAT_RESERVED_SECTORS;
        fat_format_fat(buffer, current_block, data_size_byte);
    }
    else if (block < offset_dir)
    {
        /* Root directory clusters */
        uint64_t current_block = block;
        current_block -= offset_fat;
        fat_format_dir(buffer, current_block, data_size_byte);
    }
    else if (block < offset_file)
    {
        /*File clusters */
        printf("File\n");

        uint64_t current_block = block;
        current_block -= offset_dir;
        // Return the relative block of this sector
        return current_block;
    }

    // Tell the function client that this is not file data
    return BUFFER_CHANGED;
}
