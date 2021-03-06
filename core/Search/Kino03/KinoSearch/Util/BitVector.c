#include "Search/Kino03/KinoSearch/Util/ToolSet.h"

#include <math.h>

#include "Search/Kino03/KinoSearch/Util/BitVector.h"
#include "Search/Kino03/KinoSearch/Util/I32Array.h"
#include "Search/Kino03/KinoSearch/Util/IntArrays.h"

#define BITVEC_GROW(self, num) \
    do { \
        if (num >= self->cap) \
            BitVec_Grow(self, num); \
    } while (0)

/* Shared subroutine for performing both OR and XOR ops.
 */
#define DO_OR 1
#define DO_XOR 2
static void
S_do_or_or_xor(BitVector *self, const BitVector *other, int operation);

/* 1 bit per byte.  Use bitwise and to see if a bit is set. 
 */

/* Clear a bit.  Caller must ensure that tick is within capacity.
 */
#define CLEAR(self, tick) \
    self->bits[ (tick >> 3) ] &= ~(kino_IntArr_u1masks[tick & 0x7])

/* Number of 1 bits given a u8 value. 
 */
static const u32_t BYTE_COUNTS[256] = {
    0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4,
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
    3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
    4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8
};


BitVector*
BitVec_new(u32_t capacity) 
{
    BitVector *self = (BitVector*)VTable_Make_Obj(&BITVECTOR);
    return BitVec_init(self, capacity);
}

BitVector*
BitVec_init(BitVector *self, u32_t capacity)
{
    const u32_t byte_size = (u32_t)ceil(capacity / 8.0);

    /* Derive. */
    self->bits     = CALLOCATE(byte_size, u8_t);

    /* Assign. */
    self->cap      = byte_size * 8;

    return self;
}

void
BitVec_destroy(BitVector* self) 
{
    free(self->bits);
    FREE_OBJ(self);
}

BitVector*
BitVec_clone(BitVector *self) 
{
    BitVector *evil_twin = BitVec_new(self->cap);
    u32_t byte_size      = (u32_t)ceil(self->cap / 8.0);

    /* Forbid inheritance. */
    if (self->vtable != (VTable*)&BITVECTOR)
        THROW("Attempt by %o to inherit BitVec_Clone", Obj_Get_Class_Name(self));

    memcpy(evil_twin->bits, self->bits, byte_size * sizeof(u8_t));

    return evil_twin;
}

u32_t
BitVec_get_cap(BitVector *self) { return self->cap; }

void
BitVec_copy(BitVector *self, BitVector *other)
{
    const u32_t my_byte_size    = (u32_t)ceil(self->cap / 8.0);
    const u32_t other_byte_size = (u32_t)ceil(other->cap / 8.0);
    if (my_byte_size > other_byte_size) {
        u32_t space = my_byte_size - other_byte_size;
        memset(self->bits + other_byte_size, 0, space);
    }
    else {
        BitVec_Grow(self, other->cap);
    }
    memcpy(self->bits, other->bits, other_byte_size);
}

void
BitVec_grow(BitVector *self, u32_t new_max) 
{
    if (new_max >= self->cap) {
        const size_t old_byte_cap  = (size_t)ceil(self->cap / 8.0); 
        const size_t new_byte_cap  = (size_t)ceil((new_max + 1) / 8.0); 
        const size_t num_new_bytes = new_byte_cap - old_byte_cap;

        self->bits = REALLOCATE(self->bits, new_byte_cap, u8_t);
        memset(self->bits + old_byte_cap, 0, num_new_bytes);
        self->cap = new_byte_cap * 8;
    }
}

void 
BitVec_set(BitVector *self, u32_t tick) 
{
    BITVEC_GROW(self, tick);
    IntArr_u1set(self->bits, tick);
}

void 
BitVec_clear(BitVector *self, u32_t tick) 
{
    if (tick >= self->cap) 
        return;
    CLEAR(self, tick);
}

void 
BitVec_clear_all(BitVector *self) 
{
    const size_t byte_size = (size_t)ceil(self->cap / 8.0);
    memset(self->bits, 0, byte_size);
}

bool_t
BitVec_get(BitVector *self, u32_t tick) 
{
    if (tick >= self->cap)
        return false;
    return IntArr_u1get(self->bits, tick);
}

static i32_t
S_first_bit_in_nonzero_byte(u8_t num)
{
    i32_t first_bit = 0;
    if ((num & 0xF) == 0) { first_bit += 4; num >>= 4; }
    if ((num & 0x3) == 0) { first_bit += 2; num >>= 2; }
    if ((num & 0x1) == 0) { first_bit += 1; }
    return first_bit;
}

i32_t
BitVec_next_set_bit(BitVector *self, u32_t tick) 
{
    size_t byte_size = (size_t)ceil(self->cap / 8.0);
    u8_t *const limit = self->bits + byte_size;
    u8_t *ptr = self->bits + (tick >> 3);

    if (ptr >= limit) {
        return -1;
    }
    else if (*ptr != 0) {
        /* Special case the first byte. */
        const i32_t base = (ptr - self->bits) * 8;
        const i32_t min_sub_tick = tick & 0x7;
        unsigned int byte = *ptr >> min_sub_tick;
        if (byte) {
            const i32_t candidate = base + min_sub_tick +
                S_first_bit_in_nonzero_byte(byte);
            return candidate < (i32_t)self->cap ? candidate : -1;
        }
    }

    for (ptr++ ; ptr < limit; ptr++) {
        if (*ptr != 0) {
            /* There's a non-zero bit in this byte. */
            const i32_t base = (ptr - self->bits) * 8;
            const i32_t candidate = base + S_first_bit_in_nonzero_byte(*ptr);
            return candidate < (i32_t)self->cap ? candidate : -1;
        }
    }

    return -1;
}

void
BitVec_and(BitVector *self, const BitVector *other) 
{
    u8_t *bits_a = self->bits;
    u8_t *bits_b = other->bits;
    const u32_t min_cap = self->cap < other->cap 
        ? self->cap 
        : other->cap;
    const size_t byte_size = (size_t)ceil(min_cap / 8.0);
    u8_t *const limit = bits_a + byte_size;

    /* Intersection. */
    while (bits_a < limit) {
        *bits_a &= *bits_b;
        bits_a++, bits_b++;
    }

    /* Set all remaining to zero. */
    if (self->cap > min_cap) {
        const size_t self_byte_size = (size_t)ceil(self->cap / 8.0);
        memset(bits_a, 0, self_byte_size - byte_size);
    }
}

void
BitVec_or(BitVector *self, const BitVector *other) 
{
    S_do_or_or_xor(self, other, DO_OR);
}

void
BitVec_xor(BitVector *self, const BitVector *other) 
{
    S_do_or_or_xor(self, other, DO_XOR);
}

static void
S_do_or_or_xor(BitVector *self, const BitVector *other, int operation)
{
    u8_t *bits_a, *bits_b;
    u32_t max_cap, min_cap;
    u8_t *limit;
    size_t byte_size;

    /* Sort out what the minimum and maximum caps are. */
    if (self->cap < other->cap) {
        max_cap = other->cap;
        min_cap = self->cap;
    }
    else {
        max_cap = self->cap;
        min_cap = other->cap;
    }

    /* Grow self if smaller than other, then calc pointers. */
    BITVEC_GROW(self, max_cap);
    bits_a        = self->bits;
    bits_b        = other->bits;
    byte_size     = (size_t)ceil(min_cap / 8.0);
    limit         = self->bits + byte_size;

    /* Perform union of common bits. */
    if (operation == DO_OR) {
        while (bits_a < limit) {
            *bits_a |= *bits_b;
            bits_a++, bits_b++;
        }
    }
    else if (operation == DO_XOR) {
        while (bits_a < limit) {
            *bits_a ^= *bits_b;
            bits_a++, bits_b++;
        }
    }
    else {
        THROW("Unrecognized operation: %i32", (i32_t)operation);
    }

    /* Copy remaining bits if other is bigger than self. */
    if (other->cap > min_cap) {
        const size_t other_byte_size = (size_t)ceil(other->cap / 8.0);
        const size_t bytes_to_copy = other_byte_size - byte_size;
        memcpy(bits_a, bits_b, bytes_to_copy);
    }
}

void
BitVec_and_not(BitVector *self, const BitVector *other) 
{
    u8_t *bits_a = self->bits;
    u8_t *bits_b = other->bits;
    const u32_t min_cap = self->cap < other->cap 
        ? self->cap 
        : other->cap;
    const size_t byte_size = (size_t)ceil(min_cap / 8.0);
    u8_t *const limit = bits_a + byte_size;

    /* Clear bits set in other. */
    while (bits_a < limit) {
        *bits_a &= ~(*bits_b);
        bits_a++, bits_b++;
    }
}

void
BitVec_flip(BitVector *self, u32_t tick) 
{
    const u32_t byte_offset = tick >> 3;
    const u8_t single_bit_mask = IntArr_u1masks[ (tick % 8) ];
    u8_t byte;

    BITVEC_GROW(self, tick);
    byte = self->bits[byte_offset];

    if ((byte & single_bit_mask) == single_bit_mask) /* bit is set */
        byte &= ~single_bit_mask; /* turn off one bit */
    else 
        byte |= single_bit_mask; /* turn on one bit */

    self->bits[byte_offset] = byte; 
}

void
BitVec_flip_block(BitVector *self, u32_t offset, u32_t length) 
{
    u32_t first = offset;
    u32_t last  = offset + length - 1;

    /* Proceed only if we have bits to flip. */
    if (!length) 
        return;

    BITVEC_GROW(self, last);

    /* Flip partial bytes. */
    while (last % 8 != 0 && last > first) {
        BitVec_flip(self, last);
        last--;
    }
    while (first % 8 != 0 && first < last) {
        BitVec_flip(self, first);
        first++;
    }

    /* Are first and last equal? */
    if (first == last) {
        /* There's only one bit left to flip. */
        BitVec_flip(self, last);
    }
    /* They must be multiples of 8, then. */
    else {
        const u32_t start_tick = first >> 3;
        const u32_t limit_tick = last  >> 3;
        u8_t *bits  = self->bits + start_tick;
        u8_t *limit = self->bits + limit_tick;

        /* Last actually belongs to the following byte (e.g. 8, in byte 2). */
        BitVec_flip(self, last);

        /* Flip whole bytes. */
        while (bits < limit) {
            *bits = ~(*bits);
            bits++;
        }
    }
}

u32_t 
BitVec_count(BitVector *self) 
{
    u32_t count = 0;
    const size_t byte_size = (size_t)ceil(self->cap / 8.0);
    u8_t *ptr = self->bits;
    u8_t *const limit = ptr + byte_size;

    for( ; ptr < limit; ptr++) {
        count += BYTE_COUNTS[*ptr];
    }

    return count;
}

I32Array*
BitVec_to_array(BitVector *self)
{
    u32_t count             = BitVec_Count(self);
    u32_t num_left          = count;
    const u32_t capacity    = self->cap;
    u32_t *const array      = (u32_t *const)CALLOCATE(count, u32_t);
    const size_t byte_size  = (size_t)ceil(self->cap / 8.0);
    u8_t *const bits        = self->bits;
    u8_t *const limit       = bits + byte_size;
    u32_t num               = 0;
    u32_t i                 = 0;

    while (num_left) {
        u8_t *ptr = bits + (num >> 3);
        while (ptr < limit && *ptr == 0) {
            num += 8;
            ptr++;
        }
        do {
            if (BitVec_Get(self, num)) {
                array[i++] = num;
                if (--num_left == 0)
                    break;
            }
            if (num >= capacity)
                THROW("Exceeded capacity: %u32 %u32", num, capacity);
        } while (++num % 8);
    }

    return I32Arr_new_steal((i32_t*)array, count);
}

/* Copyright 2006-2009 Marvin Humphrey
 *
 * This program is free software; you can redistribute it and/or modify
 * under the same terms as Perl itself.
 */

