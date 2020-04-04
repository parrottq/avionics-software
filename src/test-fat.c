#include <stdio.h>
#include <string.h>

#include "fat.h"

uint8_t buffer[512];

int call_all_blocks(FILE *fp)
{
    uint32_t size = 7;
    //fat_get_size(callback, &size);
    printf("Capacity %i\n", size);

    for (uint32_t i = 0; i < size; i++)
    {
        if (i < 100)
        {
            printf("Sector %i\n", i);
        }
        uint64_t sector = fat_translate_sector(i, size * 512, buffer);
        if (sector != ~((uint64_t)0))
        {
            // Is part of the file sector is offset
            char ch = 64; // @
            memset(buffer, ch, 512);
        }

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
