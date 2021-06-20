//
//  PGRingBuffer.c
//  PGRingBuffer
//
//  Created by Galen Rhodes on 8/13/20.
//  Copyright © 2020 Project Galen. All rights reserved.
//

#include "include/PGRingBuffer.h"

#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"
#define PG_ALWAYS_INLINE __attribute__((__always_inline__))

typedef void (*PGFunc)(void *, long, int);

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"

void _PGSwap16(void *words, long length, int alt) {
    long ws = (length / 2);

    if(ws) {
        uint16_t *buff = words;
        for(long i     = 0; i < ws; words++) {
            uint16_t w = buff[i];
            buff[i] = ((uint16_t)((w & 0xff00) >> 8) | (uint16_t)((w & 0x00ff) << 8));
        }
    }
}

#pragma clang diagnostic pop

void _PGSwap32(void *words, long length, int alt) {
    long ws = (length / 4);
    if(ws) {
        uint32_t *buff = words;
        for(long i     = 0; i < ws; i++) {
            uint32_t w = buff[i];
            buff[i] = (alt ? (((w & 0xffff0000) >> 16) | ((w & 0x0000ffff) << 16)) : (((w & 0xff000000) >> 24) | ((w & 0x00ff0000) >> 8) | ((w & 0x0000ff00) << 8) | ((w & 0x000000ff) << 24)));
        }
    }
}

PG_ALWAYS_INLINE uint64_t zaphod(uint64_t w, int i) {
    switch(i) {
        case 1: return (((w & 0xffffffff00000000) >> 32) | ((w & 0x00000000ffffffff) << 32));
        case 2: return (((w & 0xffff000000000000) >> 48) | ((w & 0x0000ffff00000000) >> 16) | ((w & 0x00000000ffff0000) << 16) | ((w & 0x000000000000ffff) << 48));
        default: return ((w >> 56) | ((w >> 40) & 0xff00) | ((w >> 24) & 0xff0000) | ((w >> 8) & 0xff000000) | ((w & 0xff000000) << 8) | ((w & 0xff0000) << 24) | ((w & 0xff00) << 40) | (w << 56));
    }
}

void _PGSwap64(void *buffer, long length, int alt) {
    uint64_t *words = buffer;
    long     ws     = (length / 8);

    if(ws) for(long i = 0; i < ws; ++i) words[i] = zaphod(words[i], alt);
}

PG_ALWAYS_INLINE long _PGSwapRingBufferEndian(PGRingBuffer *buff, long bytesPerWord, PGFunc func, int alt) {
    long ws = (PGRingBufferCount(buff) / bytesPerWord);

    if(ws) {
        long    length = (ws * bytesPerWord);
        uint8_t *words = malloc((size_t)length);
        PGReadFromRingBuffer(buff, words, length);
        func(words, length, alt);
        PGPrependToRingBuffer(buff, words, length);
    }

    return ws;
}

PG_ALWAYS_INLINE long pg_Min(long x, long y) {
    return ((x < y) ? x : y);
}

PG_ALWAYS_INLINE long pgIncHead(PGRingBuffer *buff, long length) {
    return ((length > 0) ? (buff->head = ((buff->head + length) % buff->size)) : buff->head);
}

PG_ALWAYS_INLINE long pgIncTail(PGRingBuffer *buff, long length) {
    return (buff->tail = ((buff->tail + length) % buff->size));
}

PG_ALWAYS_INLINE long pgDecHead(PGRingBuffer *buff, long length) {
    return (buff->head = (((buff->head < length) ? (buff->size + buff->head) : buff->head) - length));
}

PG_ALWAYS_INLINE long pgReadFrom(PGRingBuffer *buff, long start, uint8_t *dest, long length) {
    return PGMemCpy(dest, (buff->buffer + start), length);
}

PG_ALWAYS_INLINE long pgRead(PGRingBuffer *buff, uint8_t *dest, long d) {
    pgReadFrom(buff, buff->head, dest, d);
    pgIncHead(buff, d);
    return d;
}

PG_ALWAYS_INLINE long getNewBufferSize(const PGRingBuffer *buff, long needed, long nsize) {
    long c = (PGRingBufferCount(buff) + 1);
    do { nsize *= 2; } while((nsize - c) < needed);
    return nsize;
}

PG_ALWAYS_INLINE void defragBufferAfterResize(PGRingBuffer *buff, long nsize, long osize, long ohead, long otail) {
    if(buff->tail < buff->head) {
        long    hsz = (osize - ohead);
        uint8_t *nb = buff->buffer;

        if(hsz > otail) {
            // Defrag by moving the tail.
            PGMemMove((nb + osize), nb, otail);
            buff->tail += osize;
        }
        else {
            // Defrag by moving the head.
            long nhead = (nsize - hsz);
            PGMemMove((nb + nhead), (nb + ohead), hsz);
            buff->head = nhead;
        }
    }
}

bool PGDefragRingBuffer(PGRingBuffer *buff) {
    long    h  = buff->head;
    long    t  = buff->tail;
    long    s  = buff->size;
    uint8_t *b = buff->buffer;

    if(t < h) {
        long hs = (s - h);
        long ts = t;

        if(hs < ts) {
            uint8_t *nb = malloc((size_t)hs);

            if(nb) {
                PGMemMove(nb, b + h, hs);
                PGMemCpy(b + hs, b, ts);
                PGMemMove(b, nb, hs);
                buff->head = 0;
                buff->tail = (hs + ts);
                free(nb);
            }
            else {
                return false;
            }
        }
        else {
            uint8_t *nb = malloc((size_t)ts);

            if(nb) {
                PGMemMove(nb, b, ts);
                PGMemCpy(b, (b + h), hs);
                PGMemMove((b + hs), nb, ts);
                buff->head = 0;
                buff->tail = (hs + ts);
                free(nb);
            }
            else {
                return false;
            }
        }
    }
    else if(h < t) {
        if(h) {
            long cc = (t - h);
            PGMemCpy(b, (b + h), cc);
            buff->head = 0;
            buff->tail = cc;
        }
    }
    else {
        buff->head = buff->tail = 0;
    }
    return true;
}

uint8_t *PGGetRingBufferBuffer(PGRingBuffer *buff, long *size) {
    if(PGDefragRingBuffer(buff)) {
        *size = PGRingBufferCount(buff);
        return buff->buffer;
    }
    else {
        *size = 0;
        return NULL;
    }
}

long PGSwapRingBufferEndian16(PGRingBuffer *buff) {
    return _PGSwapRingBufferEndian(buff, 2, _PGSwap16, 0);
}

long PGSwapRingBufferEndian32(PGRingBuffer *buff) {
    return _PGSwapRingBufferEndian(buff, 4, _PGSwap32, 0);
}

long PGSwapRingBufferEndian32Alt(PGRingBuffer *buff) {
    return _PGSwapRingBufferEndian(buff, 4, _PGSwap32, 1);
}

long PGSwapRingBufferEndian64(PGRingBuffer *buff) {
    return _PGSwapRingBufferEndian(buff, 8, _PGSwap64, 0);
}

long PGSwapRingBufferEndian64Alt(PGRingBuffer *buff) {
    return _PGSwapRingBufferEndian(buff, 8, _PGSwap64, 1);
}

long PGSwapRingBufferEndian64AltAlt(PGRingBuffer *buff) {
    return _PGSwapRingBufferEndian(buff, 8, _PGSwap64, 2);
}

bool resizeBuffer(PGRingBuffer *buff, long needed, long osize, long ohead, long otail) {
    long    nsize = getNewBufferSize(buff, needed, osize);
    uint8_t *nb   = realloc(buff->buffer, (size_t)nsize);

    if(nb) {
        buff->buffer = nb;
        buff->size   = nsize;
        defragBufferAfterResize(buff, nsize, osize, ohead, otail);
        return true;
    }

    return false;
}

PGRingBuffer *PGCreateRingBuffer(long initialSize) {
    PGRingBuffer *buff = malloc(sizeof(PGRingBuffer));
    if(buff) {
        buff->initSize = pg_Min(initialSize, 5);
        buff->size     = buff->initSize;
        buff->head     = 0;
        buff->tail     = 0;
        buff->buffer   = malloc((size_t)buff->size);
        if(buff->buffer) return buff;
        free(buff);
        buff = NULL;
    }
    return buff;
}

void PGDiscardRingBuffer(PGRingBuffer *buff) {
    if(buff) {
        if(buff->buffer) free(buff->buffer);
        free(buff);
    }
}

long PGReadFromRingBuffer(PGRingBuffer *buff, void *dest, long maxLength) {
    if((dest != NULL) && (maxLength > 0) && (buff->head != buff->tail)) {
        if((buff->tail < buff->head)) {
            long d = pgRead(buff, dest, pg_Min(maxLength, (buff->size - buff->head)));
            return (d + pgRead(buff, (dest + d), pg_Min((maxLength - d), buff->tail)));
        }
        else {
            return pgRead(buff, dest, pg_Min(maxLength, (buff->tail - buff->head)));
        }
    }

    return 0;
}

long PGReadLastFromRingBuffer(PGRingBuffer *buff, void *dest, long maxLength) {
    if((dest != NULL) && (maxLength > 0) && (buff->head != buff->tail)) {
        if(buff->head < buff->tail) {
            long sz = pg_Min((buff->tail - buff->head), maxLength);
            buff->tail = (buff->tail - sz);
            return pgReadFrom(buff, buff->tail, dest, sz);
        }
        else {
            long sz = pg_Min(buff->tail, maxLength);
            long x  = (maxLength - pgReadFrom(buff, (buff->tail = 0), dest, sz));
            if(x > 0) {
                long sz2 = pg_Min((buff->size - buff->head), x);
                return (sz + pgReadFrom(buff, (buff->tail = (buff->size - sz2)), (dest + sz), sz2));
            }
            return sz;
        }
    }

    return 0;
}

bool PGEnsureCapacity(PGRingBuffer *buff, long needed) {
    return (bool)(((needed > 0) && (PGRingBufferRemaining(buff) < needed)) ? resizeBuffer(buff, needed, buff->size, buff->head, buff->tail) : true);
}

bool PGAppendRingBufferToRingBuffer(PGRingBuffer *dest, const PGRingBuffer *src) {
    if(src && (src->head != src->tail)) {
        if(src->head < src->tail) {
            return PGAppendToRingBuffer(dest, (src->buffer + src->head), (src->tail - src->head));
        }
        else {
            if(PGAppendToRingBuffer(dest, (src->buffer + src->head), (src->size - src->head))) {
                return PGAppendToRingBuffer(dest, src->buffer, src->tail);
            }
            return false;
        }
    }
    return true;
}

bool PGAppendToRingBuffer(PGRingBuffer *buff, const void *src, long length) {
    if(src && length > 0) {
        if(PGEnsureCapacity(buff, length)) {
            if((buff->tail < buff->head)) {
                PGMemCpy((buff->buffer + buff->tail), src, length);
                pgIncTail(buff, length);
            }
            else {
                long l = pg_Min(length, (buff->size - buff->tail));
                PGMemCpy((buff->buffer + buff->tail), src, l);
                pgIncTail(buff, l);
                return PGAppendToRingBuffer(buff, (src + l), (length - l));
            }

            return true;
        }

        return false;
    }

    return true;
}

bool PGAppendByteToRingBuffer(PGRingBuffer *buff, uint8_t byte) {
    if(PGEnsureCapacity(buff, 1)) {
        buff->buffer[buff->tail] = byte;
        pgIncTail(buff, 1);
        return true;
    }
    return false;
}

bool PGPrependRingBufferToRingBuffer(PGRingBuffer *dest, const PGRingBuffer *src) {
    if(src && (src->head != src->tail)) {
        if(src->head < src->tail) {
            return PGPrependToRingBuffer(dest, (src->buffer + src->head), (src->tail - src->head));
        }
        else {
            if(PGPrependToRingBuffer(dest, (src->buffer + src->head), (src->size - src->head))) {
                return PGPrependToRingBuffer(dest, src->buffer, src->tail);
            }
            return false;
        }
    }
    return true;
}

bool PGPrependToRingBuffer(PGRingBuffer *buff, const void *src, long length) {
    if(src && length > 0) {
        if(PGEnsureCapacity(buff, length)) {
            long ohead = buff->head;
            pgDecHead(buff, length);

            if(buff->head < ohead) {
                PGMemCpy((buff->buffer + buff->head), src, length);
            }
            else {
                long l = (buff->size - buff->head);
                PGMemCpy((buff->buffer + buff->head), src, l);
                PGMemCpy(buff->buffer, (src + l), (length - l));
            }
            return true;
        }
        return false;
    }
    return true;
}

bool PGPrependByteToRingBuffer(PGRingBuffer *buff, uint8_t byte) {
    if(PGEnsureCapacity(buff, 1)) {
        buff->buffer[pgDecHead(buff, 1)] = byte;
        return true;
    }
    return false;
}

/**
 * The current capacity of the buffer when empty.
 *
 * @param buff the buffer.
 * @return The current capacity of the buffer when empty.
 */
long PGRingBufferCapacity(const PGRingBuffer *buff) {
    return (buff->size - 1);
}

/**
 * Effectively reads and forgets the next `length` bytes from the buffer. If the number of bytes in the buffer is less than or equal to `length` then the buffer will be empty after this operation.
 *
 * @param buff the buffer.
 * @param length the number of bytes to consume from the buffer.
 */
void PGRingBufferConsume(PGRingBuffer *buff, long length) {
    if(length > 0) { pgIncHead(buff, pg_Min(PGRingBufferCount(buff), length)); }
}

/**
 * The number of bytes in the buffer.
 *
 * @param buff the buffer.
 * @return the number of bytes in the buffer.
 */
long PGRingBufferCount(const PGRingBuffer *buff) {
    return ((buff->head <= buff->tail) ? (buff->tail - buff->head) : ((buff->size - buff->head) + buff->tail));
}

/**
 * The room remaining in the buffer.
 *
 * @param buff the buffer.
 * @return the room remaining in the buffer.
 */
long PGRingBufferRemaining(const PGRingBuffer *buff) {
    return (PGRingBufferCapacity(buff) - PGRingBufferCount(buff));
}

/**
 * Get bytes from the buffer without removing them.
 *
 * @param buff the buffer.
 * @param dest the destination buffer.
 * @param maxLength the length of the destination buffer.
 * @return the number of bytes read.
 */
long PGPeekFromRingBuffer(PGRingBuffer *buff, void *dest, long maxLength) {
    long head = buff->head;
    long tail = buff->tail;
    long cc   = PGReadFromRingBuffer(buff, dest, maxLength);
    buff->head = head;
    buff->tail = tail;
    return cc;
}

bool PGClearRingBuffer(PGRingBuffer *buff, bool keepCapacity) {
    buff->head = 0;
    buff->tail = 0;

    if(!keepCapacity) {
        uint8_t *b = realloc(buff->buffer, (size_t)buff->initSize);
        if(b) {
            buff->buffer = b;
            buff->size   = buff->initSize;
        }
        else return false;
    }
    return true;
}

long PGMemCpy(void *dst, const void *src, long length) {
    if(length > 0) memcpy(dst, src, (size_t)length);
    return length;
}

long PGMemMove(void *dst, const void *src, long length) {
    if(length > 0) memmove(dst, src, (size_t)length);
    return length;
}

void PGSwap16(void *buffer, long length) {
    _PGSwap16(buffer, length, 0);
}

void PGSwap32(void *buffer, long length) {
    _PGSwap32(buffer, length, 0);
}

void PGSwap32Alt(void *buffer, long length) {
    _PGSwap32(buffer, length, 1);
}

void PGSwap64(void *buffer, long length) {
    _PGSwap64(buffer, length, 0);
}

void PGSwap64Alt(void *buffer, long length) {
    _PGSwap64(buffer, length, 1);
}

void PGSwap64AltAlt(void *buffer, long length) {
    _PGSwap64(buffer, length, 2);
}

long PGHostByteOrder(void) {
    return __BYTE_ORDER__;
}

long PGLittleEndianByteOrder(void) {
    return __ORDER_LITTLE_ENDIAN__;
}

long PGBigEndianByteOrder(void) {
    return __ORDER_BIG_ENDIAN__;
}

#pragma clang diagnostic pop
