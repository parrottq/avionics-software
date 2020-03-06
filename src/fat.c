/**
 * @file fat.c
 * @desc Format memory chucks with FAT32
 * @author Quinn Parrott
 * @date 2020-03-04
 * Last Author:
 * Last Edited On:
 */

#include "fat.h"

uint8_t fat_translate_sector(uint64_t block, struct fat_file *file, void (*structure_callback)(struct fat_builder_state *), uint8_t *buffer)
{
    if(block == 0) {
        /* Boot sector */
        
        /* Not a file block */
        file->block = 0xffff;

        struct fat_boot_sector_head *sector = (struct fat_boot_sector_head *) buffer;
        sector->BS_jmpBoot[0] = 0xeb;
        sector->BS_jmpBoot[1] = 0x58;
        sector->BS_jmpBoot[2] = 0x90;

        // TODO: Give name
        sector-> BS_OEMName = 0;
        sector->BPB_BytsPerSec = 512;
        sector->BPB_SecPerClus = 1;
        // TODO: Reserved chucks check
        sector->BPB_RsvdSecCnt = 32;
        // TODO: No second fat
        sector->BPB_NumFATs = 2;
    }
    return 0;
}

uint8_t fat_get_size(void (*structure_callback)(struct fat_builder_state *), uint32_t *size)
{
    return 0;
}

void fat_add_file(struct fat_builder_state *state, uint16_t id, char *name_str, uint64_t size)
{
}

void fat_add_directory(struct fat_builder_state *state, uint16_t id, char *name_str)
{
}