#include <unittest.h>

#ifdef SOURCE_C
#include SOURCE_C
#else
#include "fat.h"
#endif


int main(void)
{
    ut_assert(0 == INTEGER_DIVISION_ROUND_UP(0, 2));
    ut_assert(1 == INTEGER_DIVISION_ROUND_UP(1, 2));
    ut_assert(1 == INTEGER_DIVISION_ROUND_UP(2, 2));
    ut_assert(2 == INTEGER_DIVISION_ROUND_UP(3, 2));

    // Test for order of operations problem
    ut_assert(2 == INTEGER_DIVISION_ROUND_UP(3, 1 * 2));
}
