#include <stdio.h>

#include "fat.h"

uint8_t buffer[512];

enum dirs
{
    ROOT,
    DIR1,
    DIR_COUNT,
};

enum files
{
    FILE1,
    FILE2,
};

void callback(struct fat_builder_state *state, uint32_t *max_dir)
{
    *max_dir = DIR_COUNT;
    if (state == NULL)
    {
        return;
    }
    switch (state->directory)
    {
    case ROOT:
        fat_add_file(state, FILE1, "base", 1);
        fat_add_directory(state, DIR1, "dir1");
        break;
    case DIR1:
        fat_add_file(state, FILE2, "stuff", 1);
        break;
    };
}

int call_all_blocks(FILE *fp)
{
    uint32_t size = 0;
    fat_get_size(callback, &size);
    printf("Capacity %i\n", size);

    for (uint32_t i = 0; i < size; i++)
    {
        if (i < 100)
        {
            printf("Sector %i\n", i);
        }
        struct fat_file requested_block;
        fat_translate_sector(i, &requested_block, callback, buffer);

        switch (requested_block.id)
        {
        case FILE1:
            break;
        case FILE2:
            break;
        };

        fwrite(buffer, sizeof(uint8_t), sizeof(buffer), fp);
    }
}

int main(void)
{
    FILE *fp;
    fp = fopen("/tmp/test.fat32", "w");
    if (fp == NULL)
    {
        printf("Failed to create file\n");
        return 1;
    }

    call_all_blocks(fp);

    fclose(fp);
    return 0;
}
