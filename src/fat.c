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
static const uint64_t rollover = (((uint64_t)1) << 32) / (FAT_SECTOR_SIZE * FAT_SECTOR_PER_CLUSTER);

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

void fat_format_boot(uint8_t *buffer, uint64_t data_size_byte)
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
    /* Removable media */
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

    for (uint8_t i = 0; i < 12; i++)
    {
        sector->BPB_Reserved[i] = 0;
    }

    sector->BS_DrvNum = 0;
    sector->BS_Reserved1 = 0;
    sector->BS_BootSig = 0x29;
    sector->BS_VolID = 0;
    const char fat_volume_label[] = "MCU Board  ";
    memcpy(sector->BS_VolLab, fat_volume_label, sizeof(sector->BS_VolLab));
    const char fat_fs_type[] = "FAT32      ";
    memcpy(&sector->BS_FilSysType, fat_fs_type, sizeof(sector->BS_FilSysType));

    buffer[510] = 0x55;
    buffer[511] = 0xAA;
}

void fat_format_fsinfo(uint8_t *buffer)
{
    /* FSinfo */
    printf("FSInfo\n");

    buffer[0] = 0x52;
    buffer[1] = 0x52;
    buffer[2] = 0x61;
    buffer[3] = 0x41;

    buffer[510] = 0x55;
    buffer[511] = 0xAA;

    struct fat_file_system_info *sector = (struct fat_file_system_info *)(buffer + 484);
    sector->FSI_StrucSig = 0x61417272;
    sector->FSI_Free_Count = 0xffffffff;
    sector->FSI_Nxt_Free = 0xffffffff;
}

void fat_format_fat(uint8_t *buffer, uint64_t current_block, uint64_t data_size_byte)
{
    /* FAT sectors */
    printf("FAT\n");

    uint64_t file_size_cluster = fat_calc_file_size_cluster(data_size_byte);
    uint64_t dir_size_cluster = fat_calc_dir_size_cluster(data_size_byte);

    // TODO: Change order so files is first checked

    uint64_t fat_entry = 0;
    fat_entry += current_block * (FAT_SECTOR_SIZE / FAT_ENTRY_SIZE);
    for (uint32_t entry_offset = 0; entry_offset < FAT_SECTOR_SIZE; entry_offset += FAT_ENTRY_SIZE)
    {
        uint32_t *entry = (uint32_t *)(buffer + entry_offset);
        if (fat_entry < FAT_CLUSTER_OFFSET)
        {
            printf("\tRes cluster: ");
            if (fat_entry == 0)
            {
                *entry = 0x0ffffff8;
                printf("Res 1\n");
            }
            else if (fat_entry == 1)
            {
                *entry = 0x0fffffff;
                printf("Res 2\n");
            }
        }
        else if (fat_entry < (FAT_CLUSTER_OFFSET + dir_size_cluster))
        {
            printf("\tDIR entry\n");
            // Directory entry
            uint64_t fat_dir_entry = fat_entry - FAT_CLUSTER_OFFSET;
            if (fat_dir_entry == (dir_size_cluster - 1))
            {
                *entry = 0x0fffffff;
            }
            else
            {
                *entry = fat_entry + 1;
            }
        }
        else
        {
            printf("\tFile entry: ");
            // File entry
            uint64_t fat_file_entry = fat_entry - (dir_size_cluster + FAT_CLUSTER_OFFSET);
            printf("%li\n", fat_file_entry);
            if (fat_file_entry >= file_size_cluster)
            {
                // Outside the data bound, mark unallocated
                *entry = 0;
            }
            else if ((fat_file_entry % rollover) == (rollover - 1) || fat_file_entry == (file_size_cluster - 1))
            {
                // End of the file
                // Can be triggered by exceeding a file size (rollover) or the end of the data bound
                *entry = 0x0fffffff;
            }
            else
            {
                // In data bound, point to next entry to make clusters continuous
                *entry = fat_entry + 1;
            }
        }
        printf("\t\tSector offset: %li Entry: %li Value: %li\n", current_block, fat_entry, *entry);
        fat_entry++;
    }
}

void fat_format_dir(uint8_t *buffer, uint64_t current_block, uint64_t data_size_byte)
{
    /* DIR clusters */
    printf("DIR\n");

    uint64_t dir_size_cluster = fat_calc_dir_size_cluster(data_size_byte);

    // TODO: The internal if statement is completely unecessary
    const uint16_t dir_entries_per_sector = (FAT_SECTOR_SIZE / FAT_DIR_ENTRY_SIZE);
    uint32_t dir_entry = dir_entries_per_sector * current_block;
    for (uint16_t dir_entry_offset = 0; dir_entry_offset < FAT_SECTOR_SIZE; dir_entry_offset += sizeof(struct fat_directory))
    {
        struct fat_directory *entry = (struct fat_directory *)(buffer + dir_entry_offset);

        if (data_size_byte > (dir_entry * rollover * FAT_SECTOR_PER_CLUSTER * FAT_SECTOR_SIZE))
        {
            printf("\tEntry %i Offset %i\n", dir_entry, dir_entry_offset);

            // Name of the file is just a number in hex for easy sorting
            snprintf(entry->DIR_Name, sizeof(entry->DIR_Name), "%08x   ", dir_entry);
            entry->DIR_Name[10] = ' ';

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
            printf("\tStart cluster: %u file_size_left_bytes: %lu size: %u\n", start_cluster, file_size_left_bytes, entry->DIR_FileSize);
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

uint64_t fat_translate_sector(uint64_t block, uint64_t size, uint8_t *buffer)
{
    // TODO: Can we clear this better, maybe the layer above handles this
    for (uint16_t i = 0; i < 512; i++)
    {
        buffer[i] = 0;
    }

    // TODO: Pass this to the functions to reduce recalculation
    uint64_t data_size_byte = size;
    uint64_t file_size_cluster = fat_calc_file_size_cluster(size);
    uint64_t dir_size_cluster = fat_calc_dir_size_cluster(size);
    uint32_t fat_size_sector = fat_calc_fat_size_sector(file_size_cluster, dir_size_cluster);

    // TODO: Can this pattern be done better
    // TODO: Change the order that these are checked
    if (block == 0)
    {
        fat_format_boot(buffer, data_size_byte);
    }
    else if (block == 1)
    {
        fat_format_fsinfo(buffer);
    }
    else if (block < FAT_RESERVED_SECTORS)
    {
        printf("Reserved sector\n");
    }
    else if (block < (FAT_RESERVED_SECTORS + fat_size_sector))
    {
        uint64_t current_block = block;
        current_block -= FAT_RESERVED_SECTORS;
        fat_format_fat(buffer, current_block, data_size_byte);
    }
    else if (block < (FAT_RESERVED_SECTORS + fat_size_sector + FAT_SECTOR_PER_CLUSTER * dir_size_cluster))
    {
        uint64_t current_block = block;
        current_block -= FAT_RESERVED_SECTORS;
        current_block -= fat_size_sector;
        fat_format_dir(buffer, current_block, data_size_byte);
    }
    else if (block < (FAT_RESERVED_SECTORS + fat_size_sector + FAT_SECTOR_PER_CLUSTER * (dir_size_cluster + file_size_cluster)))
    {
        /*File clusters */
        printf("File\n");

        uint64_t current_block = block;
        current_block -= FAT_RESERVED_SECTORS;
        current_block -= fat_size_sector;
        current_block -= FAT_SECTOR_PER_CLUSTER * dir_size_cluster;

        return current_block;
    }
    else
    {
        // Empty sector
    }

    return ~((uint64_t)0);
}