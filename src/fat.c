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

// TODO: Some of these constants should be in fat-standard.h
/* Boot (1) + FSInfo (1) */
#define FAT_CLUSTER_OFFSET (128 * 2)

#define FAT_SECTOR_SIZE 512
#define FAT_DIR_ENTRY_SIZE 32
#define FAT_CLUSTER_ENTRY_SIZE 4

// The brackets around each variable are necessary so inline math works correctly
#define INTEGER_DIVISION_ROUND_UP(dividend, divisor) (((dividend) / (divisor)) + (((dividend) % (divisor)) > 0))

uint64_t fat_translate_sector(uint64_t block, uint64_t size, uint8_t *buffer)
{
    // TODO: Can we clear this better, maybe the layer above handles this
    uint8_t is_file_sector = 0;
    printf("%li\n", block);
    for (uint16_t i = 0; i < 512; i++)
    {
        buffer[i] = 0;
    }
    // TODO: Too much nesting break into functions
    if (block == 0)
    {
        printf("Boot block\n");
        /* Boot sector */

        /* Not a file block */
        is_file_sector = 0;

        struct fat_boot_sector_head *sector = (struct fat_boot_sector_head *)buffer;
        sector->BS_jmpBoot[0] = 0xeb;
        sector->BS_jmpBoot[1] = 0x58;
        sector->BS_jmpBoot[2] = 0x90;

        sector->BPB_BytsPerSec = FAT_SECTOR_SIZE;
        sector->BPB_SecPerClus = 1;
        // TODO: Reserved chucks check
        sector->BPB_RsvdSecCnt = FAT_CLUSTER_OFFSET;
        // TODO: No second fat
        sector->BPB_NumFATs = 1;
        sector->BPB_RootEntCnt = 0;
        sector->BPB_TotSec16 = 0;
        /* Removable media */
        sector->BPB_Media = 0x0f;
        sector->BPB_FATSz16 = 0;
        sector->BPB_SecPerTrk = 32;
        sector->BPB_NumHeads = 64;
        sector->BPB_HiddSec = 0;
        // TODO: Set this to something
        uint32_t total_sectors = 0;
        sector->BPB_TotSec32 = total_sectors; // TODO: Verify that fat_get_size not what's needed

        sector->BPB_FATSz32 = INTEGER_DIVISION_ROUND_UP(total_sectors * FAT_CLUSTER_ENTRY_SIZE, FAT_SECTOR_SIZE); // TODO: total sec * 4 (32bits) / 512
        sector->BPB_ExtFlags = 0;
        sector->BPB_FSVer = 0;
        sector->BPB_RootClus = 2; // TODO: Hard coded value
        sector->BPB_FSInfo = 1;
        sector->BPB_BkBootSec = 0;

        for (uint8_t i = 0; i < 12; i++)
        {
            sector->BPB_Reserved[i] = 0;
        }

        sector->BS_DrvNum = 0;
        sector->BS_Reserved1 = 0;
        sector->BS_BootSig = 0;
        sector->BS_VolID = 0;
        for (uint8_t i = 0; i < 11; i++)
        {
            sector->BS_VolLab[i] = ' ';
        }
        const char fat_volume_label[] = "FAT32      ";
        memcpy(sector->BS_VolLab, fat_volume_label, 11);
        memcpy(&sector->BS_FilSysType, fat_volume_label, 8);
        const char oem_name[] = "CuInSpac";
        memcpy(&sector->BS_OEMName, oem_name, 8);

        buffer[510] = 0x55;
        buffer[511] = 0xAA;
    }
    else if (block == 1)
    {
        /* FSinfo */
        printf("FSInfo\n");

        /* Not a file block */
        is_file_sector = 0;

        buffer[0] = 0x52;
        buffer[1] = 0x52;
        buffer[2] = 0x61;
        buffer[3] = 0x41;

        buffer[510] = 0x55;
        buffer[511] = 0xAA;

        struct fat_file_system_info *sector = (struct fat_file_system_info *)buffer + 484;
        sector->FSI_StrucSig = 0x61417272;
        sector->FSI_Free_Count = 0xffffffff;
        sector->FSI_Nxt_Free = 0xffffffff;
    }
    // TODO: Better reserved region handling
    // Entering cluster zone
    else if (block >= FAT_CLUSTER_OFFSET)
    {
        printf("Dyn Section: ");
        block -= FAT_CLUSTER_OFFSET;
        // TODO: Document all these variables
        uint64_t data_size_byte = size;
        uint64_t file_size_cluster = INTEGER_DIVISION_ROUND_UP(data_size_byte, 512 * 128);
        uint64_t dir_size_cluster = 1;
        uint64_t cluster_count = dir_size_cluster + file_size_cluster + INTEGER_DIVISION_ROUND_UP(file_size_cluster, 128 * 512 / 4);
        printf("Cluster count: %li\n", cluster_count);
        uint32_t fat_size_cluster = INTEGER_DIVISION_ROUND_UP(cluster_count, 128 * 512 / 4);
        printf("FAT cluster size: %li\n", fat_size_cluster);
        const uint64_t rollover = 3;
        // TODO: Change order so files is first checked
        if (block < (fat_size_cluster * 128))
        {
            // FAT
            printf("FAT\n");

            is_file_sector = 0;

            // FAT clusters
            // 128 entries per sector
            uint64_t fat_entry = 0;
            fat_entry += block * 128;
            for (uint32_t entry_offset = 0; entry_offset < 512; entry_offset += 4)
            {
                uint32_t *entry = (uint32_t *)buffer + entry_offset;
                if (fat_entry < fat_size_cluster)
                {
                    printf("FAT\n");
                    // FAT entry
                    if ((fat_size_cluster - 1) == fat_entry)
                    {
                        // End of cluster chain
                        *entry = 0xffffffff;
                    }
                    else
                    {
                        // Point to next cluster
                        *entry = fat_entry + 1;
                    }
                }
                else if (fat_entry < (fat_size_cluster + dir_size_cluster))
                {
                    printf("DIR\n");
                    // Directory entry
                    *entry = 0xffffffff;
                }
                else
                {
                    printf("File ");
                    // File entry
                    uint64_t fat_file_entry = fat_entry - (fat_size_cluster + dir_size_cluster);
                    printf("%li\n", fat_file_entry);
                    // Rollover is the number of cluster per file

                    if (fat_file_entry > file_size_cluster)
                    {
                        // Outside the data bound, mark unallocated
                        *entry = 0;
                    }
                    else if (fat_file_entry % rollover == rollover - 1 || fat_file_entry == file_size_cluster)
                    {
                        // End of the file
                        // Can be triggered by exceeding a file size (rollover) or the end of the data bound
                        *entry = 0xffffffff;
                    }
                    else
                    {
                        // In data bound, point to next entry to make clusters continuous
                        *entry = fat_entry + 1;
                    }
                }
                printf("\tSector: %li Entry: %li Value: %li\n", block + 2, fat_entry, *entry);
                fat_entry++;
            }

            // TODO: Move 128 constants somewhere
        }
        else if (block < ((fat_size_cluster + dir_size_cluster) * 128))
        {
            printf("DIR\n");
            // DIR cluster
            const uint16_t dir_entries_per_sector = (512 / 32);
            uint32_t dir_entry = dir_entries_per_sector * (block - (fat_size_cluster * 128));
            for (uint16_t dir_entry_offset = 0; dir_entry_offset < 512; dir_entry_offset += sizeof(struct fat_directory))
            {
                struct fat_directory *entry = (struct fat_directory *)buffer + dir_entry_offset;
                if (data_size_byte > (dir_entry * rollover * 128 * 512))
                {
                    // TODO: First 2 entries should be '.' and '..'

                    // TODO: Check if formated correctly
                    // Name of the file is just a number
                    snprintf(entry->DIR_Name, sizeof(entry->DIR_Name), "%08x   ", dir_entry);
                    entry->DIR_Name[11] = ' ';
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
                    uint32_t start_cluster = dir_entry * rollover;
                    entry->DIR_FstClusHI = (uint16_t)(start_cluster >> 16) & 0xffff;
                    entry->DIR_FstClusLO = (uint16_t)start_cluster & 0xffff;

                    // The size of the file
                    uint64_t file_size_left_bytes = data_size_byte - (dir_entry * rollover * 128 * 512);
                    if (file_size_left_bytes > (512 * 128))
                    {
                        entry->DIR_FileSize = rollover * 128 * 512;
                    }
                    else
                    {
                        entry->DIR_FileSize = file_size_left_bytes;
                    }
                    printf("Start cluster: %u file_size_left_bytes: %lu size: %u\n", start_cluster, file_size_left_bytes, entry->DIR_FileSize);
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
        else
        {
            // File
            printf("FILE\n");
        }
    }

    // File clusters
    if (is_file_sector == 0)
    {
        // TODO: Make this mean something
        return 0;
    }
    else
    {
        return ~((uint64_t)0);
    }
}