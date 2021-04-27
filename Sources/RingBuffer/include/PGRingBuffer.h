//
//  PGRingBuffer.h
//  PGRingBuffer
//
//  Created by Galen Rhodes on 8/13/20.
//  Copyright © 2020 Project Galen. All rights reserved.
//

#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"
#ifndef PGRingBuffer_h
#define PGRingBuffer_h

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stddef.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

__BEGIN_DECLS

typedef struct _st_pg_ringbuffer_ {
    long    initSize;
    long    size;
    long    head;
    long    tail;
    uint8_t *buffer;
}               PGRingBuffer;

#define PG_EXPORT extern __attribute__((__visibility__("default")))

PG_EXPORT long PGHostByteOrder(void);

PG_EXPORT long PGLittleEndianByteOrder(void);

PG_EXPORT long PGBigEndianByteOrder(void);

/**
 * Creates and initializes a new ring buffer.
 *
 * @param initialSize the initial size of the ring buffer. If less then five then the default is five.
 * @return the newly created ring buffer.
 */
PG_EXPORT PGRingBuffer *PGCreateRingBuffer(long initialSize);

/**
 * Deallocates an existing ring buffer.
 *
 * @param buff the ring buffer to deallocate.
 */
PG_EXPORT void PGDiscardRingBuffer(PGRingBuffer *buff);

PG_EXPORT bool PGDefragRingBuffer(PGRingBuffer *buff);

PG_EXPORT uint8_t *PGGetRingBufferBuffer(PGRingBuffer *buff, long *size);

/**
 * Reads up to `maxLength` bytes from the ring buffer into `dest`. If `maxLength` is more then the number of
 * bytes in the ring buffer then those bytes will be read and the actual number read will be returned.
 *
 * @param buff the ring buffer.
 * @param dest the destination buffer.
 * @param maxLength the size of the destination buffer.
 * @return the number of bytes actually read.
 */
PG_EXPORT long PGReadFromRingBuffer(PGRingBuffer *buff, void *dest, long maxLength);

/**
 * Get bytes from the buffer without removing them.
 *
 * @param buff the buffer.
 * @param dest the destination buffer.
 * @param maxLength the length of the destination buffer.
 * @return the number of bytes read.
 */
PG_EXPORT long PGPeekFromRingBuffer(PGRingBuffer *buff, void *dest, long maxLength);

/**
 * Ensures that the ring buffer has enough capacity to accept the `needed` number of new bytes.
 * If there is not enough capacity then the size of the buffer is doubled until there is enough.
 *
 * @param buff the buffer.
 * @param needed the number of new bytes to be added.
 * @return `true` if there is enough room or the resize was successful. `false` if there was not
 *         enough capacity and there was not enough memory to resize the buffer.
 */
PG_EXPORT bool PGEnsureCapacity(PGRingBuffer *buff, long needed);

/**
 * Append the given bytes to the end of the ring buffer - resizing the buffer if needed.
 *
 * @param buff the buffer.
 * @param src the source bytes.
 * @param length the number of bytes to append.
 * @return `true` if successful or `false` if buffer size could not be expanded due to lack of memory.
 */
PG_EXPORT bool PGAppendToRingBuffer(PGRingBuffer *buff, const void *src, long length);

/**
 * Appends the contents of the source ring buffer to the end of the destination ring buffer -
 * resizing if needed.
 *
 * @param dest the destination ring buffer.
 * @param src the source ring buffer.
 * @return `true` if successful or `false` if buffer size could not be expanded due to lack of memory.
 */
PG_EXPORT bool PGAppendRingBufferToRingBuffer(PGRingBuffer *dest, const PGRingBuffer *src);

/**
 * Prepends the contents of the source ring buffer to the beginning of the destination ring buffer -
 * resizing if needed.
 *
 * @param dest the destination ring buffer.
 * @param src the source ring buffer.
 * @return `true` if successful or `false` if buffer size could not be expanded due to lack of memory.
 */
PG_EXPORT bool PGPrependRingBufferToRingBuffer(PGRingBuffer *dest, const PGRingBuffer *src);

/**
 * Append a single byte to the end of the ring buffer - resizing the buffer if needed.
 *
 * @param buff the buffer.
 * @param byte the byte.
 * @return `true` if successful or `false` if buffer size could not be expanded due to lack of memory.
 */
PG_EXPORT bool PGAppendByteToRingBuffer(PGRingBuffer *buff, uint8_t byte);

/**
 * Prepends the given bytes to the beginning of the ring buffer - resizing the buffer if needed.
 *
 * @param buff the buffer.
 * @param src the source bytes.
 * @param length the number of bytes to prepend.
 * @return `true` if successful or `false` if buffer size could not be expanded due to lack of memory.
 */
PG_EXPORT bool PGPrependToRingBuffer(PGRingBuffer *buff, const void *src, long length);

/**
 * Prepend a single byte to the end of the ring buffer - resizing the buffer if needed.
 *
 * @param buff the buffer.
 * @param byte the byte.
 * @return `true` if successful or `false` if buffer size could not be expanded due to lack of memory.
 */
PG_EXPORT bool PGPrependByteToRingBuffer(PGRingBuffer *buff, uint8_t byte);

/**
 * Clears the buffer.
 *
 * @param buff the buffer.
 * @param keepCapacity if true then the capacity is maintained. If false then
 *                     the buffer is shrunk back to it's initial size.
 * @return `true` if successful, `false` if th buffer could not be resized.
 */
PG_EXPORT bool PGClearRingBuffer(PGRingBuffer *buff, bool keepCapacity);

/**
 * While treating the ring buffer as a series of 16-bit words, this function
 * will swap the order of the high and low bytes of each word. If there are an
 * odd number of bytes then the last byte is ignored.
 *
 * @param buff the ring buffer.
 * @return the number of 16-bit words that had their byte order swapped.
 */
PG_EXPORT long PGSwapRingBufferEndian16(PGRingBuffer *buff);

/**
 * While treating the ring buffer as a series of 32-bit words, this function
 * will swap the order of the high and low bytes of each word. If the number of
 * bytes in the ring buffer is not divisible by 4 then the last word in the
 * ring buffer is ignored.
 *
 * This function performs the swapping in the standard (most common) way as follows:
 *
 *   +---+---+---+---+       +---+---+---+---+
 *   | A | B | C | D |  ==>  | D | C | B | A |
 *   +---+---+---+---+       +---+---+---+---+
 *
 * @param buff the ring buffer.
 * @return the number of 32-bit words that had their byte order swapped.
 */
PG_EXPORT long PGSwapRingBufferEndian32(PGRingBuffer *buff);

/**
 * While treating the ring buffer as a series of 32-bit words, this function
 * will swap the order of the high and low bytes of each word. If the number of
 * bytes in the ring buffer is not divisible by 4 then the last word in the
 * ring buffer is ignored.
 *
 * This function performs the swapping in the non-standard (least common) way
 * as follows:
 *
 *   +---+---+---+---+       +---+---+---+---+
 *   | A | B | C | D |  ==>  | C | D | A | B |
 *   +---+---+---+---+       +---+---+---+---+
 *
 * @param buff the ring buffer.
 * @return the number of 32-bit words that had their byte order swapped.
 */
PG_EXPORT long PGSwapRingBufferEndian32Alt(PGRingBuffer *buff);

/**
 * While treating the ring buffer as a series of 64-bit words, this function
 * will swap the order of the high and low bytes of each word. If the number of
 * bytes in the ring buffer is not divisible by 8 then the last word in the
 * ring buffer is ignored.
 *
 * This function performs the swapping in the standard (most common) way as follows:
 *
 *   +---+---+---+---+---+---+---+---+       +---+---+---+---+---+---+---+---+
 *   | A | B | C | D | E | F | G | H |  ==>  | H | G | F | E | D | C | B | A |
 *   +---+---+---+---+---+---+---+---+       +---+---+---+---+---+---+---+---+
 *
 * @param buff the ring buffer.
 * @return the number of 64-bit words that had their byte order swapped.
 */
PG_EXPORT long PGSwapRingBufferEndian64(PGRingBuffer *buff);

/**
 * While treating the ring buffer as a series of 64-bit words, this function
 * will swap the order of the high and low bytes of each word. If the number of
 * bytes in the ring buffer is not divisible by 8 then the last word in the
 * ring buffer is ignored.
 *
 * This function performs the swapping in the non-standard (least common) way as follows:
 *
 *   +---+---+---+---+---+---+---+---+       +---+---+---+---+---+---+---+---+
 *   | A | B | C | D | E | F | G | H |  ==>  | E | F | G | H | A | B | C | D |
 *   +---+---+---+---+---+---+---+---+       +---+---+---+---+---+---+---+---+
 *
 * @param buff the ring buffer.
 * @return the number of 64-bit words that had their byte order swapped.
 */
PG_EXPORT long PGSwapRingBufferEndian64Alt(PGRingBuffer *buff);

/**
 * While treating the ring buffer as a series of 64-bit words, this function
 * will swap the order of the high and low bytes of each word. If the number of
 * bytes in the ring buffer is not divisible by 8 then the last word in the
 * ring buffer is ignored.
 *
 * This function performs the swapping in the non-standard (least common) way as follows:
 *
 *   +---+---+---+---+---+---+---+---+       +---+---+---+---+---+---+---+---+
 *   | A | B | C | D | E | F | G | H |  ==>  | G | H | E | F | C | D | A | B |
 *   +---+---+---+---+---+---+---+---+       +---+---+---+---+---+---+---+---+
 *
 * @param buff the ring buffer.
 * @return the number of 64-bit words that had their byte order swapped.
 */
PG_EXPORT long PGSwapRingBufferEndian64AltAlt(PGRingBuffer *buff);

/**
 * Returns the current TOTAL capacity of the ring buffer as if it were empty.
 *
 * @param buff the buffer.
 * @return the current total capacity.
 */
PG_EXPORT long PGRingBufferCapacity(const PGRingBuffer *buff);


/**
 * Effectively reads and forgets the next `length` bytes from the buffer. If the number of bytes in the buffer is less than or equal to `length` then the buffer will be empty after this operation.
 *
 * @param buff the buffer.
 * @param length the number of bytes to consume from the buffer.
 */
PG_EXPORT void PGRingBufferConsume(PGRingBuffer *buff, long length);

/**
 * Returns the number of bytes currently in the ring buffer ready to be read.
 *
 * @param buff the buffer.
 * @return the number of bytes in the buffer.
 */
PG_EXPORT long PGRingBufferCount(const PGRingBuffer *buff);

/**
 * Returns the number of bytes that the ring buffer is ready to accept without having to be resized.
 *
 * @param buff the buffer.
 * @return the number of bytes the buffer can currently accept without resizing.
 */
PG_EXPORT long PGRingBufferRemaining(const PGRingBuffer *buff);

/**
 * While treating `buffer` as a series of 16-bit words, the function will swap the order of the high and low bytes of each word.
 * If `length` is an odd number then the last byte is ignored.
 *
 * @param buffer the buffer.
 * @param length the length of the buffer counted as BYTES.
 */
PG_EXPORT void PGSwap16(void *buffer, long length);

/**
 * While treating `buffer` as a series of 32-bit words, this function
 * will swap the order of the bytes of each word. If `length` is not divisible
 * by 4 then the last word in the `buffer` is ignored.
 *
 * This function performs the swapping in the standard (most common) way as follows:
 *
 *   +---+---+---+---+       +---+---+---+---+
 *   | A | B | C | D |  ==>  | D | C | B | A |
 *   +---+---+---+---+       +---+---+---+---+
 *
 * @param buffer the `buffer`.
 * @param length the number of BYTES in the `buffer`.
 */
PG_EXPORT void PGSwap32(void *buffer, long length);

/**
 * While treating `buffer` as a series of 32-bit words, this function
 * will swap the order of the bytes of each word. If `length` is not divisible
 * by 4 then the last word in the `buffer` is ignored.
 *
 * This function performs the swapping in the non-standard (least common) way as follows:
 *
 *   +---+---+---+---+       +---+---+---+---+
 *   | A | B | C | D |  ==>  | C | D | A | B |
 *   +---+---+---+---+       +---+---+---+---+
 *
 * @param buffer the `buffer`.
 * @param length the number of BYTES in the `buffer`.
 */
PG_EXPORT void PGSwap32Alt(void *buffer, long length);

/**
 * While treating `buffer` as a series of 64-bit words, this function
 * will swap the order of the bytes of each word. If `length` is not divisible
 * by 8 then the last word in the `buffer` is ignored.
 *
 * This function performs the swapping in the standard (most common) way as follows:
 *
 *   +---+---+---+---+---+---+---+---+       +---+---+---+---+---+---+---+---+
 *   | A | B | C | D | E | F | G | H |  ==>  | H | G | F | E | D | C | B | A |
 *   +---+---+---+---+---+---+---+---+       +---+---+---+---+---+---+---+---+
 *
 * @param buffer the `buffer`.
 * @param length the number of BYTES in the `buffer`.
 */
PG_EXPORT void PGSwap64(void *buffer, long length);

/**
 * While treating `buffer` as a series of 64-bit words, this function
 * will swap the order of the bytes of each word. If `length` is not divisible
 * by 8 then the last word in the `buffer` is ignored.
 *
 * This function performs the swapping in the non-standard (least common) way as follows:
 *
 *   +---+---+---+---+---+---+---+---+       +---+---+---+---+---+---+---+---+
 *   | A | B | C | D | E | F | G | H |  ==>  | E | F | G | H | A | B | C | D |
 *   +---+---+---+---+---+---+---+---+       +---+---+---+---+---+---+---+---+
 *
 * @param buffer the `buffer`.
 * @param length the number of BYTES in the `buffer`.
 */
PG_EXPORT void PGSwap64Alt(void *buffer, long length);

/**
 * While treating `buffer` as a series of 64-bit words, this function
 * will swap the order of the bytes of each word. If `length` is not divisible
 * by 8 then the last word in the `buffer` is ignored.
 *
 * This function performs the swapping in the VERY non-standard (VERY least common) way as follows:
 *
 *   +---+---+---+---+---+---+---+---+       +---+---+---+---+---+---+---+---+
 *   | A | B | C | D | E | F | G | H |  ==>  | G | H | E | F | C | D | A | B |
 *   +---+---+---+---+---+---+---+---+       +---+---+---+---+---+---+---+---+
 *
 * @param buffer the `buffer`.
 * @param length the number of BYTES in the `buffer`.
 */
PG_EXPORT void PGSwap64AltAlt(void *buffer, long length);

/**
 * Copies `length` bytes from `src` to `dst` as if first copied to an intermediate buffer.
 *
 * @param dst the destination
 * @param src the source
 * @param length the number of bytes to copy
 * @return the number of bytes copied.
 */
PG_EXPORT long PGMemCpy(void *dst, const void *src, long length);

/**
 * Copies `length` bytes from `src` to `dst` directly. The behaviour is undefined if any of the
 * two regions of `src` and `dst` overlap.
 *
 * @param dst the destination
 * @param src the source
 * @param length the number of bytes to copy
 * @return the number of bytes copied.
 */
PG_EXPORT long PGMemMove(void *dst, const void *src, long length);

__END_DECLS

#endif /* PGRingBuffer_h */

#pragma clang diagnostic pop
