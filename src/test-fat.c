#include <stdio.h>

#include "fat.h"


uint8_t buffer[512];

int call_all_blocks(FILE *fp) {
    uint32_t size = 0;
    fat_get_size(callback, &size);

    for (uint32_t i = 0; i < size; i++ ){
        fwrite(buffer, sizeof(uint8_t), sizeof(buffer), fp);
    }
}

int main(void) {

    FILE *fp;
    fp = fopen("test.fat32", "w");
    if (fp == NULL) {
        printf("Failed to create file");
        return 1;
    }

    fclose(fp);
    return 0;
}
