#include "Search/Kino03/KinoSearch/Util/ToolSet.h"

#include "Search/Kino03/KinoSearch/Analysis/Inversion.h"
#include "Search/Kino03/KinoSearch/Analysis/Token.h"

/* After inversion, record how many like tokens occur in each group.
 */
static void
S_count_clusters(Inversion *self);

Inversion*
Inversion_new(Token *seed_token) 
{
    Inversion *self = (Inversion*)VTable_Make_Obj(&INVERSION);

    /* Init. */
    self->cap                 = 16;
    self->size                = 0;
    self->tokens              = CALLOCATE(self->cap, Token*);
    self->cur                 = 0;
    self->inverted            = false;
    self->cluster_counts      = NULL;
    self->cluster_counts_size = 0;

    /* Process the seed token. */
    if (seed_token != NULL) {
        Inversion_append(self, (Token*)INCREF(seed_token));
    }

    return self;
}

void            
Inversion_destroy(Inversion *self)
{       
    if (self->tokens) {
        Token **tokens       = self->tokens;
        Token **const limit  = tokens + self->size;
        for ( ; tokens < limit; tokens++) {
            DECREF(*tokens);
        }
        free(self->tokens);
    }
    free(self->cluster_counts);
    FREE_OBJ(self);
}

Token*
Inversion_next(Inversion *self) 
{
    /* Kill the iteration if we're out of tokens. */
    if (self->cur == self->size)
        return NULL;
    return self->tokens[ self->cur++ ];
}

void
Inversion_reset(Inversion *self) 
{
    self->cur = 0;
}

static void
S_grow(Inversion *self, size_t capacity) 
{
    if (capacity > self->cap) {
        self->tokens = REALLOCATE(self->tokens, capacity, Token*); 
        self->cap    = capacity;
        memset(self->tokens + self->size, 0,
            (capacity - self->size) * sizeof(Token*));
    }
}

void
Inversion_append(Inversion *self, Token *token) 
{
    /* Safety check. */
    if (self->inverted)
        THROW("Can't append tokens after inversion");

    /* Minimize reallocations. */
    if (self->size >= self->cap) {
        if (self->cap < 100) {
            S_grow(self, 100);
        }
        else if (self->size < 10000) {
            S_grow(self, self->cap * 2);
        }
        else {
            S_grow(self, self->cap + 10000);
        }
    }

    /* Inlined VA_Push. */
    self->tokens[ self->size ] = token;
    self->size++;
}

Token**
Inversion_next_cluster(Inversion *self, u32_t *count)
{
    Token **cluster = self->tokens + self->cur;

    if (self->cur == self->size) {
        *count = 0;
        return NULL;
    }

    /* Don't read past the end of the cluster counts array. */
    if (!self->inverted)
        THROW("Inversion not yet inverted");
    if (self->cur > self->cluster_counts_size)
        THROW("Tokens were added after inversion");

    /* Place cluster count in passed-in var, advance bookmark. */
    *count = self->cluster_counts[ self->cur ];
    self->cur += *count;

    return cluster;
}

void 
Inversion_invert(Inversion *self)
{
    Token **tokens = self->tokens;
    Token **limit  = tokens + self->size;
    i32_t   token_pos = 0;

    /* Thwart future attempts to append. */
    if (self->inverted)
        THROW("Inversion has already been inverted");
    self->inverted = true;

    /* Assign token positions. */
    for ( ;tokens < limit; tokens++) {
        Token *const cur_token = *tokens;
        cur_token->pos = token_pos;
        token_pos += cur_token->pos_inc;
        if (token_pos < cur_token->pos) {
            THROW("Token positions out of order: %i32 %i32", 
                cur_token->pos, token_pos);
        }
    }

    /* Sort the tokens lexically, and hand off to cluster counting routine. */
    qsort(self->tokens, self->size, sizeof(Token*), Token_compare);
    S_count_clusters(self);
}

static void
S_count_clusters(Inversion *self)
{
    Token **tokens      = self->tokens;
    u32_t  *counts      = CALLOCATE(self->size + 1, u32_t); 
    u32_t   i;

    /* Save the cluster counts. */
    self->cluster_counts_size = self->size;
    self->cluster_counts = counts;

    for (i = 0; i < self->size; ) {
        Token *const base_token = tokens[i];
        char  *const base_text  = base_token->text;
        const size_t base_len   = base_token->len;
        u32_t j = i + 1;

        /* Iterate through tokens until text doesn't match. */
        while (j < self->size) {
            Token *const candidate = tokens[j];

            if (   (candidate->len == base_len)
                && (memcmp(candidate->text, base_text, base_len) == 0)
            ) {
                j++;
            }
            else {
                break;
            }
        }

        /* Record a count at the position of the first token in the cluster. */
        counts[i] = j - i;

        /* Start the next loop at the next token we haven't seen. */
        i = j;
    }
}

/* Copyright 2006-2009 Marvin Humphrey
 *
 * This program is free software; you can redistribute it and/or modify
 * under the same terms as Perl itself.
 */

