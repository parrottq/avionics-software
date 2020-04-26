/**
 * @file math_util.h
 * @desc Macros for common arithmetic functions
 * @author Quinn Parrott
 * @date 2020-04-26
 * Last Author:
 * Last Edited On:
 */

#ifndef math_util_h
#define math_util_h

/**
 * Clamp the given value at a maximum value.
 *
 * @param value Value to clamp
 * @param max_value Value that will not be exceeded
 *
 * @return Clamped value
 */
#define CLAMP_MAX(value, max_value) ((value) > (max_value) ? (max_value) : (value))

/**
 * Similar to integer division except the quotient is rounded up instead of down.
 * Equivalent to ceil(A / B).
 *
 * @param dividend A in the division "A / B"
 * @param divisor B in the division "A / B"
 *
 * @return The quotient of a devision rounded up
 */
#define INTEGER_DIVISION_ROUND_UP(dividend, divisor) (((dividend) / (divisor)) + (((dividend) % (divisor)) > 0))
// The brackets around each variable are necessary so inline math works correctly

#endif /* math_util_h */
