#include <unittest.h>

#ifdef SOURCE_C
#include SOURCE_C
#else
#include "fat.h"
#endif

#include <string.h>

/*
 * fat_add_file() is one of the functions available in a structure_callback
 * that modifies the supplied fat_builder_state depending on what type of
 * pass is being done internally.
 */

int main(void)
{
    // FAT_BUILDER_PASS_TYPE_TOTAL_CHUNKS on fat_add_file()
    {
        // Basic pass
        struct fat_builder_state state;
        state.pass_type = FAT_BUILDER_PASS_TYPE_TOTAL_CHUCKS;
        state.chunk_count = 0;
        state.file_dir_count = 0;
        const uint32_t dir = 1;
        state.directory = dir;

        fat_add_file(&state, dir, "empty", 1);

        ut_assert(state.chunk_count == 1);
        ut_assert(state.file_dir_count == 1);
    }

    {
        // Ignore if not selected directory
        struct fat_builder_state state;
        state.pass_type = FAT_BUILDER_PASS_TYPE_TOTAL_CHUCKS;
        state.chunk_count = 0;
        state.file_dir_count = 0;
        const uint32_t dir = 1;
        state.directory = 0; // Note not 'dir'

        fat_add_file(&state, dir, "empty", 1);

        ut_assert(state.chunk_count == 0);
        ut_assert(state.file_dir_count == 0);
    }

    {
        // Different sized file
        struct fat_builder_state state;
        state.pass_type = FAT_BUILDER_PASS_TYPE_TOTAL_CHUCKS;
        state.chunk_count = 0;
        state.file_dir_count = 0;
        const uint32_t dir = 1;
        state.directory = dir;

        fat_add_file(&state, dir, "empty", 10);

        ut_assert(state.chunk_count == 1);
        ut_assert(state.file_dir_count == 1);
    }

    {
        // Multiple files
        struct fat_builder_state state;
        state.pass_type = FAT_BUILDER_PASS_TYPE_TOTAL_CHUCKS;
        state.chunk_count = 0;
        state.file_dir_count = 0;
        const uint32_t dir = 1;
        state.directory = dir;

        fat_add_file(&state, dir, "empty", 1);
        fat_add_file(&state, dir, "empty", 2);

        ut_assert(state.chunk_count == 2);
        ut_assert(state.file_dir_count == 2);
    }

    {
        // Overflowing files (over 4GB limit imposed by 32bit int)
        struct fat_builder_state state;
        state.pass_type = FAT_BUILDER_PASS_TYPE_TOTAL_CHUCKS;
        state.chunk_count = 0;
        state.file_dir_count = 0;
        const uint32_t dir = 1;
        state.directory = dir;

        fat_add_file(&state, dir, "empty", 4294967296);

        printf("Got %i\n", state.chunk_count);
        ut_assert(state.chunk_count == 8388608);
        ut_assert(state.file_dir_count == 2);
    }

    {
        // Empty file
        struct fat_builder_state state;
        state.pass_type = FAT_BUILDER_PASS_TYPE_TOTAL_CHUCKS;
        state.chunk_count = 0;
        state.file_dir_count = 0;
        const uint32_t dir = 1;
        state.directory = dir;

        fat_add_file(&state, dir, "empty", 0);

        ut_assert(state.chunk_count == 1);
        ut_assert(state.file_dir_count == 1);
    }

    // FAT_BUILDER_PASS_TYPE_WRITE_FAT on fat_add_file()
    uint8_t *buffer = (uint8_t *) malloc(512);
    {
        // Basic first sector write
        struct fat_builder_state state;
        state.pass_type = FAT_BUILDER_PASS_TYPE_WRITE_FAT;
        state.chunk_count = 1;
        state.file_dir_count = 0;
        const uint32_t dir = 1;
        state.directory = dir;
        memset(buffer, 0, 512);
        state.buffer = buffer;
        state.data.write_fat.ignore_entries = 0;
        state.data.write_fat.entry_offset = 0;

        const uint64_t file_size = 1;
        fat_add_file(&state, dir, "empty", file_size);

        ut_assert(state.chunk_count == 2);
        ut_assert(state.file_dir_count == 1);
        ut_assert(state.data.write_fat.ignore_entries == 0);
        ut_assert(state.data.write_fat.entry_offset == 1);
        ut_assert(*((uint32_t *) buffer) == 1);
    }
    {
        // Multiple sectors write
        struct fat_builder_state state;
        state.pass_type = FAT_BUILDER_PASS_TYPE_WRITE_FAT;
        state.chunk_count = 2;
        state.file_dir_count = 0;
        const uint32_t dir = 1;
        state.directory = dir;
        memset(buffer, 0, 512);
        state.buffer = buffer;
        state.data.write_fat.ignore_entries = 0;
        state.data.write_fat.entry_offset = 0;

        const uint64_t file_size = 512;
        fat_add_file(&state, dir, "empty", file_size);

        ut_assert(state.chunk_count == 4);
        ut_assert(state.file_dir_count == 1);
        ut_assert(state.data.write_fat.ignore_entries == 0);
        ut_assert(state.data.write_fat.entry_offset == 2);
        ut_assert(*((uint32_t *) buffer + (sizeof(uint32_t) * 0)) == 2);
        ut_assert(*((uint32_t *) buffer + (sizeof(uint32_t) * 1)) == 3);
    }
    {
        // Ignore
        struct fat_builder_state state;
        state.pass_type = FAT_BUILDER_PASS_TYPE_WRITE_FAT;
        state.chunk_count = 1;
        state.file_dir_count = 0;
        const uint32_t dir = 1;
        state.directory = dir;
        memset(buffer, 0, 512);
        state.buffer = buffer;
        state.data.write_fat.ignore_entries = 3;
        state.data.write_fat.entry_offset = 0;

        const uint64_t file_size = 1;
        fat_add_file(&state, dir, "empty", file_size);

        ut_assert(state.chunk_count == 1);
        ut_assert(state.file_dir_count == 0);
        ut_assert(state.data.write_fat.ignore_entries == 2);
        ut_assert(state.data.write_fat.entry_offset == 0);
        ut_assert(*((uint32_t *) buffer) == 0);
    }
    {
        // Multifile
        struct fat_builder_state state;
        state.pass_type = FAT_BUILDER_PASS_TYPE_WRITE_FAT;
        state.chunk_count = 1;
        state.file_dir_count = 0;
        const uint32_t dir = 1;
        state.directory = dir;
        memset(buffer, 0, 512);
        state.buffer = buffer;
        state.data.write_fat.ignore_entries = 0;
        state.data.write_fat.entry_offset = 0;

        const uint64_t file_size = 4294967296;
        fat_add_file(&state, dir, "empty", file_size);

        ut_assert(state.chunk_count == 1+8388608);
        ut_assert(state.file_dir_count == 2);
        ut_assert(state.data.write_fat.ignore_entries == 0);
        ut_assert(state.data.write_fat.entry_offset == 2);
        char expected_buffer[512];
        for (uint32_t i = 0; i<(512/sizeof(uint32_t));i++){
            *((uint32_t *) expected_buffer+(i*sizeof(uint32_t))) = i;
        }
        ut_assert(memcmp(buffer, expected_buffer, 512) == 0);
    }
    // Not first sector of fat
    // Out of bounds of sector
}
