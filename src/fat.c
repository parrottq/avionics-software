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

#define FAT_SECTOR_SIZE 512
#define FAT_DIR_ENTRY_SIZE 32

#define INTEGER_DIVISION_ROUND_UP(dividend, divisor) ((dividend / divisor) + ((dividend % divisor) > 0))

/**
 * Get the size of all files and directories excluding file allocation table and reserved chunks.
 */
static uint8_t fat_builder_get_capacity(void (*structure_callback)(struct fat_builder_state *), uint32_t *total_chunks)
{
    /* Set the type of pass */
    struct fat_builder_state state;
    state.pass_type = FAT_BUILDER_PASS_TYPE_TOTAL_CHUCKS;

    /* Scan each directory */
    for (uint32_t dir_index = 0; dir_index <= state.data.total_chunks.max_dir_index; dir_index++)
    {
        /* Reset values */
        state.data.total_chunks.chunk_count = 0;
        state.data.total_chunks.file_dir_count = 0;
        state.data.total_chunks.max_dir_index = 0;

        state.directory = dir_index;

        /* Call the callback to populate state */
        structure_callback(&state);

        /* Add the chunks from files */
        *total_chunks += state.data.total_chunks.chunk_count;

        /* Calculate the chunks added from directories */
        uint32_t current_directory_entries = state.data.total_chunks.file_dir_count;
        uint32_t current_directory_chunks = 0;
        if (current_directory_entries > 0)
        {
            /* Determine the number of chunks needed to represent all entries */
            current_directory_chunks = INTEGER_DIVISION_ROUND_UP(current_directory_entries, (FAT_SECTOR_SIZE / FAT_DIR_ENTRY_SIZE));
        }
        else
        {
            /* Directories must be at least one chunk big even if empty */
            current_directory_chunks = 1;
        }
        *total_chunks += current_directory_chunks;
    }

    return 0;
}

/**
 * Write all the directory/file entries into a buffer
 */
static uint8_t fat_builder_write_dir(void (*structure_callback)(struct fat_builder_state *), uint8_t *buffer, uint32_t dir, uint16_t page)
{
    /* Set the type of pass */
    struct fat_builder_state state;
    state.pass_type = FAT_BUILDER_PASS_TYPE_WRITE_DIR_CHUCK;
    state.buffer = buffer;

    state.directory = dir;
    state.data.write_dir.ignore_entries = 32 * page;
    state.data.write_dir.entry_offset = 0;

    structure_callback(&state);

    return 0;
}

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
    else if (block == 2)
    {
        // TODO: This is hard coded and should be generated based on the fs structure callback
        fat_builder_write_dir(structure_callback, buffer, 0, 0);
    }
    return 0;
}

uint8_t fat_get_size(void (*structure_callback)(struct fat_builder_state *), uint32_t *size)
{
    fat_builder_get_capacity(structure_callback, size);
    *size += FAT_CLUSTER_OFFSET;
    return 0;
}

void fat_add_file(struct fat_builder_state *state, uint16_t id, char *name_str, uint64_t size)
{
    switch (state->pass_type)
    {
    case FAT_BUILDER_PASS_TYPE_TOTAL_CHUCKS:
        /* Add file chunks using interger division that rounds up */
        state->data.total_chunks.chunk_count += INTEGER_DIVISION_ROUND_UP(size, FAT_SECTOR_SIZE);
        /* Count another directory entry */
        state->data.total_chunks.file_dir_count += INTEGER_DIVISION_ROUND_UP(size, 0xffffffff);
        break;
    case FAT_BUILDER_PASS_TYPE_WRITE_DIR_CHUCK:
        // TODO: Handle files larger than 32-bit limit
        if (state->data.write_dir.ignore_entries > 0) {
            state->data.write_dir.ignore_entries -= 1;
        } else {
            struct fat_directory *entry = (struct fat_directory *) (state->buffer + (sizeof(struct fat_directory) * state->data.write_dir.entry_offset));
            state->data.write_dir.entry_offset += 1;
            uint8_t str_len = strlen(name_str);
            if (str_len >= 11){
                memcpy(entry->DIR_Name, name_str, 11);
            } else {
                memset(entry->DIR_Name, ' ', 11);
                memcpy(entry->DIR_Name, name_str, str_len);
            }
            entry->DIR_Attr = 0;
            entry->DIR_NTRes = 0;
            entry->DIR_CrtTimeTenth = 0;
            entry->DIR_CrtTime = 0;
            entry->DIR_CrtDate = 0;
            entry->DIR_LstAccDate = 0;
            // TODO: Make this work
            entry->DIR_FstClusHI = 0;
            entry->DIR_WrtTime = 0;
            entry->DIR_WrtDate = 0;
            // TODO: Cluster implementation
            entry->DIR_FstClusLO = 0;
            entry->DIR_FileSize = size;
        }
        break;
    }
}

void fat_add_directory(struct fat_builder_state *state, uint16_t id, char *name_str)
{
    switch (state->pass_type)
    {
    case FAT_BUILDER_PASS_TYPE_TOTAL_CHUCKS:
        /* Count another directory entry */
        state->data.total_chunks.file_dir_count += 1;

        /* This is part of the search for highest directory index */
        if (state->data.total_chunks.max_dir_index < id)
        {
            state->data.total_chunks.max_dir_index = id;
        }
        break;
    case FAT_BUILDER_PASS_TYPE_WRITE_DIR_CHUCK:
        /* Handling same as file in this case except file size is zero */
        if (state->data.write_dir.ignore_entries > 0) {
            state->data.write_dir.ignore_entries -= 1;
        } else {
            struct fat_directory *entry = (struct fat_directory *) (state->buffer + (sizeof(struct fat_directory) * state->data.write_dir.entry_offset));
            state->data.write_dir.entry_offset += 1;
            uint8_t str_len = strlen(name_str);
            if (str_len >= 11){
                memcpy(entry->DIR_Name, name_str, 11);
            } else {
                memset(entry->DIR_Name, ' ', 11);
                memcpy(entry->DIR_Name, name_str, str_len);
            }
            entry->DIR_Attr = 0x10;
            entry->DIR_NTRes = 0;
            entry->DIR_CrtTimeTenth = 0;
            entry->DIR_CrtTime = 0;
            entry->DIR_CrtDate = 0;
            entry->DIR_LstAccDate = 0;
            // TODO: Make this work
            entry->DIR_FstClusHI = 0;
            entry->DIR_WrtTime = 0;
            entry->DIR_WrtDate = 0;
            // TODO: Cluster implementation
            entry->DIR_FstClusLO = 0;
            entry->DIR_FileSize = 0;
        }
        break;
    }
}