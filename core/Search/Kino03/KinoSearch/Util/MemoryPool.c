#include "Search/Kino03/KinoSearch/Util/ToolSet.h"

#include "Search/Kino03/KinoSearch/Util/MemoryPool.h"
#include "Search/Kino03/KinoSearch/Util/ByteBuf.h"

static void
S_init_arena(MemoryPool *self, size_t amount);

#define DEFAULT_BUF_SIZE 0x100000 /* 1 MiB */

/* Enlarge amount so pointers will always be aligned.
 */
#define INCREASE_TO_WORD_MULTIPLE(_amount) \
    do { \
        const size_t _remainder = _amount % sizeof(void*); \
        if (_remainder) { \
            _amount += sizeof(void*); \
            _amount -= _remainder; \
        } \
    } while (0)

MemoryPool*
MemPool_new(u32_t arena_size)
{
    MemoryPool *self = (MemoryPool*)VTable_Make_Obj(&MEMORYPOOL);

    self->arena_size = arena_size == 0 ? DEFAULT_BUF_SIZE : arena_size;
    self->arenas     = VA_new(16);
    self->tick       = -1;
    self->buf        = NULL;
    self->limit      = NULL;
    self->consumed   = 0;
    
    return self;
}

void
MemPool_destroy(MemoryPool *self)
{
    DECREF(self->arenas);
    FREE_OBJ(self);
}

static void
S_init_arena(MemoryPool *self, size_t amount)
{
    ByteBuf *bb;
    i32_t i;

    /* Indicate which arena we're using at present. */
    self->tick++;

    if (self->tick < (i32_t)VA_Get_Size(self->arenas)) {
        /* In recycle mode, use previously acquired memory. */
        bb = (ByteBuf*)VA_Fetch(self->arenas, self->tick);
        if (amount >= BB_Get_Size(bb)) {
            BB_Grow(bb, amount);
            BB_Set_Size(bb, amount);
        }
    }
    else {
        /* In add mode, get more mem from system. */
        size_t buf_size = (amount + 1) > self->arena_size 
            ? (amount + 1)
            : self->arena_size;
        char *ptr       = MALLOCATE(buf_size, char);
        if (ptr == NULL)
            THROW("Failed to allocate memory");
        bb = BB_new_steal_str(ptr, buf_size - 1, buf_size);
        VA_Push(self->arenas, (Obj*)bb);
    }

    /* Recalculate consumption to take into account blocked off space. */
    self->consumed = 0;
    for (i = 0; i < self->tick; i++) {
        ByteBuf *bb = (ByteBuf*)VA_Fetch(self->arenas, i);
        self->consumed += BB_Get_Size(bb);
    }

    self->buf   = bb->ptr;
    self->limit = BBEND(bb);
}

void*
MemPool_grab(MemoryPool *self, size_t amount)
{
    INCREASE_TO_WORD_MULTIPLE(amount);
    self->last_buf = self->buf;

    /* Verify that we have enough stocked up, otherwise get more. */
    self->buf += amount;
    if (self->buf >= self->limit) {
        /* Get enough mem from system or die trying. */
        S_init_arena(self, amount);
        self->last_buf = self->buf;
        self->buf += amount;
    }

    /* Track bytes we've allocated from this pool. */
    self->consumed += amount;

    return self->last_buf;
}

void
MemPool_resize(MemoryPool *self, void *ptr, size_t new_amount)
{
    const size_t last_amount = self->buf - self->last_buf;
    INCREASE_TO_WORD_MULTIPLE(new_amount);

    if (ptr != self->last_buf) {
        THROW("Not the last pointer allocated.");
    }
    else {
        if (new_amount <= last_amount) {
            const size_t difference = last_amount - new_amount;
            self->buf      -= difference;
            self->consumed -= difference;
        }
        else {
            THROW("Can't resize to greater amount: %u64 > %u64", 
                (u64_t)new_amount, (u64_t)last_amount);
        }
    }
}

void
MemPool_release_all(MemoryPool *self)
{
    self->tick     = -1;
    self->buf      = NULL;
    self->last_buf = NULL;
    self->limit    = NULL;
}

void
MemPool_eat(MemoryPool *self, MemoryPool *other) {
    i32_t i;
    if (self->buf != NULL)
        THROW("Memory pool is not empty");

    /* Move active arenas from other to self. */
    for (i = 0; i <= other->tick; i++) {
        ByteBuf *arena = (ByteBuf*)VA_Shift(other->arenas);
        /* Maybe displace existing arena. */
        VA_Store(self->arenas, i, (Obj*)arena); 
    }
    self->tick     = other->tick;
    self->last_buf = other->last_buf;
    self->buf      = other->buf;
    self->limit    = other->limit;
}

/* Copyright 2007-2009 Marvin Humphrey
 *
 * This program is free software; you can redistribute it and/or modify
 * under the same terms as Perl itself.
 */

