#include <unittest.h>

#ifdef SOURCE_C
#include SOURCE_C
#else
#include "fat.h"
#endif

/*
 * fat_add_file() is one of the functions available in a structure_callback
 * that modifies the supplied fat_builder_state depending on what type of
 * pass is being done internally.
 */


int main(void)
{
    // FAT_BUILDER_PASS_TYPE_TOTAL_CHUNKS on fat_add_file()
    {
        struct fat_builder_state state;
        state.pass_type = FAT_BUILDER_PASS_TYPE_TOTAL_CHUCKS;
        state.chunk_count = 0;
        state.file_dir_count = 0;

        fat_add_file(&state, 1, "empty", 1);

        ut_assert(state.chunk_count == 1);
        ut_assert(state.file_dir_count == 1);
    }
}
