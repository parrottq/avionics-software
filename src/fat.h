/**
 * @file fat.h
 * @desc Format memory chucks with FAT32
 * @author Quinn Parrott
 * @date 2020-03-04
 * Last Author:
 * Last Edited On:
 */

#ifndef fat_h
#define fat_h

//#include "global.h"
#include <stdint.h>

#include "fat-standard.h"

/**
 * 1. Directory builder
 * 2. Files
 */

/**
 * 1. Boot sector
 * -root clust
 * -fsinfo
 * -total sect
 * 2. fsinfo
 */

/*
enum {
    ROOT,
    FILE1,
    DIR1,
    FILE2,
}
void callback (struct state) {
    switch (state) {
        ROOT :{
            fat_add_file(state, "Somename.asd", len);
            fat_add_dir(state, "dir name");
        },
        DIR1 : {
            fat_add_file(state, "named", len);
        }
    }
}

void main(){
    fat_build(callback, int);
    switch (int) {
        file1: {
            write to buffer
        },
        file2: {
            write to buffer
        }
    }
    queue_transfer(buffer);
}
*/

enum fat_build/**
 * Add a directory to the fat_builder_state
 * 
 * @param state builder_state given to the callback
 * @param id Directory identification
 * @param name_str Name of the directory max 11 chars
 */er_pass_type
{
    FAT_BUILDER_PASS_TYPE_CHUCK_ALLOCATION,
    FAT_BUILDER_PASS_TYPE_WRITE_DIR_CHUCK,
};

struct fat_builder_state
{
    uint16_t directory;
    enum fat_builder_pass_type pass_type;
};

struct fat_file
{
    uint16_t id;
    uint64_t block;
};

/**
 * Translates a block number into the current file and it's block number.
 * 
 * @param block Block number that will be translated
 * @param file fat_file structure containing corresponding file.id and file.block
 * // TODO: More detailed description
 * @param structure_callback Used to describe the directory structure of the file system
 * @param buffer 512 byte buffer that is written to if the block is filesystem related
 * 
 * @note If the file.id is 0xffff then this is not a file block and unless the function failed,
 * filesystem data has been written to the buffer. 
 * 
 * @return != 0 if failure
 */
extern uint8_t fat_translate_sector(uint64_t block, struct fat_file *file, void (*structure_callback)(struct fat_builder_state *), uint8_t *buffer);

/**
 * Given the structure_callback calculate the capacity of the fat partition
 * 
 * @param structure_callback Used to describe the directory structure of the file system
 * @param size The calculated size in bytes
 * 
 * @return != 0 if failure
 */
extern uint8_t fat_get_size(void (*structure_callback)(struct fat_builder_state *), uint32_t *size);

/**
 * Add a file to the fat_builder_state
 * 
 * @param state builder_state given to the callback
 * @param id File identification number
 * @param name_str Name of the file max 11 chars
 * @param size Size of the file in bytes, this CAN be larger the 32 bit max inherent in fat32
 */
extern void fat_add_file(struct fat_builder_state *state, uint16_t id, char *name_str, uint64_t size);

/**
 * Add a directory to the fat_builder_state
 * 
 * @param state builder_state given to the callback
 * @param id Directory identification
 * @param name_str Name of the directory max 11 chars
 */
extern void fat_add_directory(struct fat_builder_state *state, uint16_t id, char *name_str);

#endif /* fat_h */