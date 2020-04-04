#include <stdio.h>
#include <string.h>

#include "fat.h"

uint8_t buffer[512];

void call_all_blocks(FILE *fp)
{
    uint64_t size = 512 * 4 * 70000;

    uint32_t total_sectors = fat_get_total_sectors(size);
    printf("Capacity %lu Sector count %u\n", size, total_sectors);

    for (uint32_t i = 0; i < total_sectors; i++)
    {
        if (i < 10000 || i % 1000 == 0)
        {
            printf("\nSector %u\n", i);
        }

        uint64_t sector = fat_translate_sector(i, size, buffer);
        if (sector != ~((uint64_t)0))
        {
            // Is part of the file sector is offset
            printf("Offset: %lu\n", sector);
            char ch = 0;
            memset(buffer, ch, 512);
            uint64_t *entry = (uint64_t *)buffer;
            *entry = sector;
        }

        fwrite(buffer, sizeof(uint8_t), sizeof(buffer), fp);
        fflush(fp);
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
