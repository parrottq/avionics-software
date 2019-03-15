/**
 * @file circular-buffer.h
 * @desc Fixed length circular buffer implementation
 * @author Samuel Dewan
 * @date 2018-12-27
 * Last Author:
 * Last Edited On:
 */

#ifndef circular_buffer_h
#define circular_buffer_h

#include "global.h"

/**
 *  Instance of an arbitrary length circular buffer.
 */
struct circular_buffer_t {
    uint8_t *buffer;
    uint16_t capacity;
    
    uint16_t head;
    uint16_t tail;
};


/**
 *  Initilize a new circular buffer from an existing array.
 *
 *  @param buffer The buffer descriptor to be initilized.
 *  @param memory The underlying array for the new buffer.
 *  @param length The length for the buffer.
 */
static inline void init_circular_buffer(struct circular_buffer_t *buffer,
                                 uint8_t *memory, uint16_t length)
{
    buffer->buffer = memory;
    buffer->capacity = length;
    
    buffer->head = 0;
    buffer->tail = 0;
}

/**
 *  Determine if a circular buffer is empty.
 *
 *  @param buffer The circular buffer for which the empty-ness should be
 *         determined.
 *
 *  @return A non-zero value if the buffer is empty, 0 otherwise
 */
static inline uint8_t circular_buffer_is_empty(struct circular_buffer_t *buffer)
{
    return buffer->tail == buffer->head;
}

/**
 *  Determine if a circular buffer is full.
 *
 *  @param buffer The circular buffer for which the full-ness should be
 *                determined.
 *
 *  @return A non-zero value if the buffer is full, 0 otherwise
 */
static inline uint8_t circular_buffer_is_full(struct circular_buffer_t *buffer)
{
    return ((buffer->tail + 1) % buffer->capacity) == buffer->head;
}

/**
 *  Determine the amount of used space in the buffer
 *
 *  @param buffer The circular buffer for which the amount of free space should
 *                be found.
 *
 *  @return The number of free bytes in the buffer.
 */
static inline uint16_t circular_buffer_unused(struct circular_buffer_t *buffer)
{
    if (circular_buffer_is_empty(buffer)) {
        return buffer->capacity;
    } else if (buffer->tail >= buffer->head) {
        return buffer->tail - buffer->head;
    } else {
        return (buffer->capacity - buffer->head) + buffer->tail;
    }
}

/**
 *  Get the capacity of a circular buffer.
 *
 *  @param buffer The circular buffer for which the capacity should be
 *                determined.
 *
 *  @return The capacity of the buffer.
 */
static inline uint8_t circular_buffer_capacity(struct circular_buffer_t *buffer)
{
    return buffer->capacity;
}

/**
 *  Insert an item at the tail of a circular buffer.
 *
 *  @note If the buffer is full, the oldest data will be overwritten.
 *
 *  @param buffer The circular buffer into which data should be inserted.
 *  @param value The data to be inserted.
 */
static inline void circular_buffer_push(struct circular_buffer_t *buffer,
                                 uint8_t value)
{
    __disable_irq();
    
    buffer->buffer[buffer->tail] = value;
    buffer->tail = (buffer->tail + 1) % buffer->capacity;
    
    if (buffer->tail == buffer->head) {
        buffer->head = (buffer->head + 1) % buffer->capacity;
    }
    
    __enable_irq();
}

/**
 *  Insert an item at the tail of a circular buffer iff there is space
 *  avaliable.
 *
 *  @param buffer The circular buffer into which data should be inserted.
 *  @param value The data to be inserted.
 *
 *  @return 0 on success, 1 if the buffer is full
 */
static inline uint8_t circular_buffer_try_push(struct circular_buffer_t *buffer,
                                        uint8_t value)
{
    if (circular_buffer_is_full(buffer)) {
        return 1;
    } else {
        circular_buffer_push(buffer, value);
        return 0;
    }
}

/**
 *  Get the item from the head of a circular buffer, if avaliable, and remove
 *  it from the buffer.
 *
 *  @param buffer The circular buffer from which an item should be popped.
 *  @param value Pointer where the popped item will be stored.
 *
 *  @return 0 on success, 1 if the buffer is empty
 */
static inline uint8_t circular_buffer_pop(struct circular_buffer_t *buffer,
                                   uint8_t *value)
{
    if (circular_buffer_is_empty(buffer)) {
        return 1;
    } else {
        __disable_irq();
        
        *value = buffer->buffer[buffer->head];
        buffer->head = (buffer->head + 1) % buffer->capacity;
        
        __enable_irq();
        return 0;
    }
}

/**
 *  Get the item from the head of a circular buffer, if avaliable, without
 *  removing it from the buffer.
 *
 *  @param buffer The circular buffer from which an item should be gotten.
 *  @param value Pointer where the peaked item will be stored.
 *
 *  @return 0 on success, 1 if the buffer is empty
 */
static inline uint8_t circular_buffer_peak(struct circular_buffer_t *buffer,
                                    uint8_t *value)
{
    if (circular_buffer_is_empty(buffer)) {
        return 1;
    } else {
        __disable_irq();
        
        *value = buffer->buffer[buffer->head];
        
        __enable_irq();
        return 0;
    }
}

/**
 *  Remove an item from the tail of the buffer.
 *
 *  @param buffer The buffer from which the last inserted item should be removed
 *
 *  @return 0 on success, 1 if the buffer is empty
 */
static inline uint8_t circular_buffer_unpush(struct circular_buffer_t *buffer)
{
    if (circular_buffer_is_empty(buffer)) {
        return 1;
    } else {
        __disable_irq();
        
        if (buffer->tail == 0) {
            buffer->tail = buffer->capacity;
        }
        buffer->tail--;
        
        __enable_irq();
        return 0;
    }
}

/**
 *  Determine if a character is present in a circular buffer.
 *
 *  @param buffer The buffer in which the presence of a character should be
 *                determined.
 *  @param c The character which should be searched for.
 *
 *  @return 1 if the character is found, 0 otherwise
 */
static inline uint8_t circular_buffer_has_char(struct circular_buffer_t *buffer,
                                               char c)
{
    for (uint16_t i = buffer->head; i != buffer->tail;
         i = ((i + 1) % buffer->capacity)) {
        if (buffer->buffer[i] == c) {
            return 1;
        }
    }
    
    return 0;
}

/**
 *  Resets a circular buffer to an empty state.
 *
 *  @param buffer The buffer to be cleared.
 */
static inline void circular_buffer_clear(struct circular_buffer_t *buffer)
{
    buffer->head = 0;
    buffer->tail = 0;
}

#endif /* circular_buffer_h */