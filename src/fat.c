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

/* Boot (1) + FSInfo (1) */
#define FAT_CLUSTER_OFFSET 2

uint8_t fat_translate_sector(uint64_t block, struct fat_file *file, void (*structure_callback)(struct fat_builder_state *), uint8_t *buffer)
{
    for (uint16_t i = 0; i < 512; i++)
    {
        buffer[i] = 0;
    }
    if (block == 0)
    {
        /* Boot sector */

        /* Not a file block */
        file->block = 0xffff;

        struct fat_boot_sector_head *sector = (struct fat_boot_sector_head *)buffer;
        sector->BS_jmpBoot[0] = 0xeb;
        sector->BS_jmpBoot[1] = 0x58;
        sector->BS_jmpBoot[2] = 0x90;

        // TODO: Give name
        sector->BS_OEMName = 0;
        sector->BPB_BytsPerSec = 512;
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
        sector->BPB_TotSec32 = 0; // TODO: Calculate total sectors

        sector->BPB_FATSz32 = 0; // TODO: total sec * 4 (32bits) / 512
        sector->BPB_ExtFlags = 0;
        sector->BPB_FSVer = 0;
        sector->BPB_RootClus = 2; // TODO: Make meaningfull
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
            sector->BS_VolLab[i] = 0;
        }
        const char fat_volume_label[] = "FAT     ";
        memcpy(sector->BS_VolLab, fat_volume_label, 8);

        buffer[510] = 0x55;
        buffer[511] = 0xAA;
    }
    else if (block == 1)
    {
        /* FSinfo */

        /* Not a file block */
        file->block = 0xffff;

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
    return 0;
}

uint8_t fat_get_size(void (*structure_callback)(struct fat_builder_state *), uint32_t *size)
{
    *size = FAT_CLUSTER_OFFSET;
    return 0;
}

void fat_add_file(struct fat_builder_state *state, uint16_t id, char *name_str, uint64_t size)
{
}

void fat_add_directory(struct fat_builder_state *state, uint16_t id, char *name_str)
{
}