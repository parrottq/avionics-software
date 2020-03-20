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

// TODO: Some of these constants should be in fat-standard.h
/* Boot (1) + FSInfo (1) */
#define FAT_CLUSTER_OFFSET 2

#define FAT_SECTOR_SIZE 512
#define FAT_DIR_ENTRY_SIZE 32
#define FAT_CLUSTER_ENTRY_SIZE 4

#define INTEGER_DIVISION_ROUND_UP(dividend, divisor) ((dividend / divisor) + ((dividend % divisor) > 0))

static uint32_t fat_entry_to_chunks(uint32_t current_directory_entries)
{
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
    return current_directory_chunks;
}

/**
 * Convert states into sector capacity
 */
static void fat_state_get_capacity(struct fat_builder_state *state, uint32_t *dir_size)
{
    /* Add the chunks from files */
    *dir_size += state->chunk_count;

    /* Calculate the chunks added from directories */
    *dir_size += fat_entry_to_chunks(state->file_dir_count);
}

/**
 * Get the size of all file and directories in a directory in chunks
 */
static void fat_builder_get_dir_capacity(void (*structure_callback)(struct fat_builder_state *, uint32_t *), struct fat_builder_state *state)
{
    state->pass_type = FAT_BUILDER_PASS_TYPE_TOTAL_CHUCKS;

    /* Reset values */
    state->chunk_count = 0;
    state->file_dir_count = 0;

    /* Call the callback to populate state */
    uint32_t dir_size = 0;
    structure_callback(state, &dir_size);
}

/**
 * Get size up until dir
 */
static uint8_t fat_builder_get_capacity_until(void (*structure_callback)(struct fat_builder_state *, uint32_t *), uint32_t *total_chunks, uint32_t max_dir)
{
    /* Set the type of pass */
    struct fat_builder_state state;
    state.pass_type = FAT_BUILDER_PASS_TYPE_TOTAL_CHUCKS;

    /* Scan each directory */
    for (uint32_t dir_index = 0; dir_index < max_dir; dir_index++)
    {
        /* Calculate the chunks added from directories */
        state.directory = dir_index;

        /* Populate the state with sector data */
        fat_builder_get_dir_capacity(structure_callback, &state);

        /* Extract it into total_chunks */
        fat_state_get_capacity(&state, total_chunks);
    }

    return 0;
}

/**
 * Get the size of all files and directories excluding file allocation table and reserved chunks.
 */
static uint8_t fat_builder_get_capacity_all(void (*structure_callback)(struct fat_builder_state *, uint32_t *), uint32_t *total_chunks)
{
    uint32_t max_dir = 0;
    structure_callback(NULL, &max_dir);

    fat_builder_get_capacity_until(structure_callback, total_chunks, max_dir);

    return 0;
}

/**
 * Write all the directory/file entries into a buffer
 */
static uint8_t fat_builder_write_dir(void (*structure_callback)(struct fat_builder_state *, uint32_t *), uint8_t *buffer, uint32_t dir, uint16_t page)
{
    /* Set the type of pass */
    struct fat_builder_state state;
    state.pass_type = FAT_BUILDER_PASS_TYPE_WRITE_DIR_CHUCK;
    state.buffer = buffer;
    state.structure_callback = structure_callback;
    state.chunk_count = 0;
    state.file_dir_count = 0;
    state.directory = dir;

    state.data.write_dir.ignore_entries = 32 * page;
    state.data.write_dir.entry_offset = 0;

    uint32_t dir_size = 0;
    structure_callback(&state, &dir_size);

    return 0;
}

uint8_t fat_translate_sector(uint64_t block, struct fat_file *file, void (*structure_callback)(struct fat_builder_state *, uint32_t *), uint8_t *buffer)
{
    // TODO: Can we clear this better, maybe the layer above handles this
    for (uint16_t i = 0; i < 512; i++)
    {
        buffer[i] = 0;
    }
    // TODO: Too much nesting break into functions
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
        uint32_t total_sectors = 0;
        fat_builder_get_capacity_all(structure_callback, &total_sectors);
        sector->BPB_TotSec32 = total_sectors; // TODO: Verify that fat_get_size not what's needed

        sector->BPB_FATSz32 = INTEGER_DIVISION_ROUND_UP(total_sectors * FAT_CLUSTER_ENTRY_SIZE, FAT_SECTOR_SIZE); // TODO: total sec * 4 (32bits) / 512
        sector->BPB_ExtFlags = 0;
        sector->BPB_FSVer = 0;
        sector->BPB_RootClus = 2; // TODO: Make meaningful
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
    // TODO: Better reserved region handling
    else if (block >= 2)
    {
        uint32_t cluster = block - FAT_CLUSTER_OFFSET;
        // TODO: FAT implement (sector allocation table)
        uint32_t total_sectors = 0;
        fat_builder_get_capacity_all(structure_callback, &total_sectors);
        uint32_t fat_size = INTEGER_DIVISION_ROUND_UP(total_sectors * FAT_CLUSTER_ENTRY_SIZE, FAT_SECTOR_SIZE);

        if (cluster < fat_size)
        {
            /* You have entered the FAT zone */
        }
        else
        {
/* You have entered the data region */
// TODO: Small modifications are needed to make this work
#if 0
            uint32_t cluster = block - 2;

            struct fat_builder_state state;

            /* Loop until this the directory is in the range of the block */
            for (uint32_t dir = 0; dir < 64; dir++)
            {
                state.directory = dir;
                fat_builder_get_dir_capacity(structure_callback, &state);

                uint32_t sectors_allocated = 0;
                fat_state_get_capacity(&state, &sectors_allocated);

                if (cluster < sectors_allocated)
                {
                    /* This is the directory */
                    break;
                }

                cluster -= sectors_allocated;
            }

            uint32_t directory_sector_count = fat_entry_to_chunks(state.file_dir_count);
            if (cluster < directory_sector_count)
            {
                /* Not a file */
                file->block = 0xffff;

                /* Directory sector */
                fat_builder_write_dir(structure_callback, buffer, state.directory, cluster);
            }
            else
            {
                /* File sector */
                // TODO: File sector translation
                file->id = 0;
                file->block = 0;
            }
#endif
        }
    }
    return 0;
}

uint8_t fat_get_size(void (*structure_callback)(struct fat_builder_state *, uint32_t *), uint32_t *size)
{
    /* Data region */
    fat_builder_get_capacity_all(structure_callback, size);
    /* FAT region */
    *size += INTEGER_DIVISION_ROUND_UP((*size * 4), FAT_SECTOR_SIZE);
    /* Reserved region */
    *size += FAT_CLUSTER_OFFSET;
    return 0;
}

void fat_add_file(struct fat_builder_state *state, uint16_t id, char *name_str, uint64_t size)
{
    if (state->directory != id)
    {
        return;
    }

    /* Add file chunks using interger division that rounds up */
    uint64_t chunks = INTEGER_DIVISION_ROUND_UP(size, FAT_SECTOR_SIZE);
    if (chunks > 0)
    {
        state->chunk_count += INTEGER_DIVISION_ROUND_UP(size, FAT_SECTOR_SIZE);
    }
    else
    {
        state->chunk_count += 1;
    }

    /* Count another directory entry */
    state->file_dir_count += size == 0 ? 1 : INTEGER_DIVISION_ROUND_UP(size, 0xffffffff);

    switch (state->pass_type)
    {
    case FAT_BUILDER_PASS_TYPE_TOTAL_CHUCKS:
        /* No special handling */
        break;
    case FAT_BUILDER_PASS_TYPE_WRITE_DIR_CHUCK:
        // TODO: Handle files larger than 32-bit limit
        if (state->data.write_dir.ignore_entries > 0)
        {
            state->data.write_dir.ignore_entries -= 1;
        }
        else
        {
            struct fat_directory *entry = (struct fat_directory *)(state->buffer + (sizeof(struct fat_directory) * state->data.write_dir.entry_offset));
            state->data.write_dir.entry_offset += 1;
            uint8_t str_len = strlen(name_str);
            if (str_len >= 11)
            {
                memcpy(entry->DIR_Name, name_str, 11);
            }
            else
            {
                memset(entry->DIR_Name, ' ', 11);
                memcpy(entry->DIR_Name, name_str, str_len);
            }
            entry->DIR_Attr = 0;
            entry->DIR_NTRes = 0;
            entry->DIR_CrtTimeTenth = 0;
            entry->DIR_CrtTime = 0;
            entry->DIR_CrtDate = 0;
            entry->DIR_LstAccDate = 0;
            entry->DIR_WrtTime = 0;
            entry->DIR_WrtDate = 0;
            entry->DIR_FileSize = size;

            uint32_t current_chunk = 0;
            fat_builder_get_capacity_until(state->structure_callback, &current_chunk, state->directory);
            current_chunk += state->chunk_count;
            // TODO: Cluster implementation (does this work?)
            entry->DIR_FstClusHI = (current_chunk >> 16) & 0xffff;
            entry->DIR_FstClusLO = current_chunk & 0xffff;
        }
        break;
    case FAT_BUILDER_PASS_TYPE_WRITE_FAT:
        if (state->data.write_fat.ignore_entries > 0) {
            state->data.write_fat.ignore_entries -= 0;
        } else {
            

            state->data.write_fat.entry_offset += 1;
        }
    }
}

void fat_add_directory(struct fat_builder_state *state, uint16_t id, char *name_str)
{
    /* Count another directory entry */
    state->file_dir_count += 1;

    switch (state->pass_type)
    {
    case FAT_BUILDER_PASS_TYPE_TOTAL_CHUCKS:
        /* No special handling */
        break;
    case FAT_BUILDER_PASS_TYPE_WRITE_DIR_CHUCK:
        /* Handling same as file in this case except file size is zero */
        if (state->data.write_dir.ignore_entries > 0)
        {
            state->data.write_dir.ignore_entries -= 1;
        }
        else
        {
            struct fat_directory *entry = (struct fat_directory *)(state->buffer + (sizeof(struct fat_directory) * state->data.write_dir.entry_offset));
            state->data.write_dir.entry_offset += 1;
            uint8_t str_len = strlen(name_str);
            // TODO: String formater function
            if (str_len >= 11)
            {
                memcpy(entry->DIR_Name, name_str, 11);
            }
            else
            {
                memset(entry->DIR_Name, ' ', 11);
                memcpy(entry->DIR_Name, name_str, str_len);
            }
            entry->DIR_Attr = 0x10;
            entry->DIR_NTRes = 0;
            entry->DIR_CrtTimeTenth = 0;
            entry->DIR_CrtTime = 0;
            entry->DIR_CrtDate = 0;
            entry->DIR_LstAccDate = 0;
            entry->DIR_WrtTime = 0;
            entry->DIR_WrtDate = 0;
            entry->DIR_FileSize = 0;

            uint32_t current_chunk = 0;
            fat_builder_get_capacity_until(state->structure_callback, &current_chunk, state->directory);
            current_chunk += state->chunk_count;
            // TODO: Cluster implementation (does this work?)
            entry->DIR_FstClusHI = (current_chunk >> 16) & 0xffff;
            entry->DIR_FstClusLO = current_chunk & 0xffff;
        }
        break;
    }
}