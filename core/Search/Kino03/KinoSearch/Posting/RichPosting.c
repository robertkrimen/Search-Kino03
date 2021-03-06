#include "Search/Kino03/KinoSearch/Util/ToolSet.h"

#include "Search/Kino03/KinoSearch/Posting/RichPosting.h"
#include "Search/Kino03/KinoSearch/Analysis/Token.h"
#include "Search/Kino03/KinoSearch/Analysis/Inversion.h"
#include "Search/Kino03/KinoSearch/Index/PostingList.h"
#include "Search/Kino03/KinoSearch/Index/PostingPool.h"
#include "Search/Kino03/KinoSearch/Posting/RawPosting.h"
#include "Search/Kino03/KinoSearch/FieldType.h"
#include "Search/Kino03/KinoSearch/Search/Similarity.h"
#include "Search/Kino03/KinoSearch/Search/Compiler.h"
#include "Search/Kino03/KinoSearch/Store/InStream.h"
#include "Search/Kino03/KinoSearch/Util/MemoryPool.h"

#define FREQ_MAX_LEN     C32_MAX_BYTES
#define MAX_RAW_POSTING_LEN(_text_len, _freq) \
        (              sizeof(RawPosting) \
                     + _text_len                /* term text content */ \
                     + FREQ_MAX_LEN             /* freq c32 */ \
                     + (C32_MAX_BYTES * _freq)  /* positions deltas */ \
                     + _freq                    /* per-pos boost byte */ \
        )

RichPosting*
RichPost_new(Similarity *sim)
{
    RichPosting *self = (RichPosting*)VTable_Make_Obj(&RICHPOSTING);
    return RichPost_init(self, sim);
}

RichPosting*
RichPost_init(RichPosting *self, Similarity *sim)
{
    ScorePost_init((ScorePosting*)self, sim);
    self->prox_boosts     = NULL;
    return self;
}

void
RichPost_destroy(RichPosting *self)
{
    free(self->prox_boosts);
    SUPER_DESTROY(self, RICHPOSTING);
}

RichPosting*
RichPost_clone(RichPosting *self)
{
    RichPosting *evil_twin = (RichPosting*)VTable_Make_Obj(self->vtable);
    evil_twin = RichPost_init(evil_twin, self->sim);

    if (self->freq) {
        evil_twin->prox_cap = evil_twin->freq;
        evil_twin->prox = MALLOCATE(evil_twin->freq, u32_t);
        evil_twin->prox_boosts = MALLOCATE(evil_twin->freq, float);
        memcpy(evil_twin->prox, self->prox, self->freq * sizeof(u32_t));
        memcpy(evil_twin->prox_boosts, self->prox_boosts, 
            self->freq * sizeof(float));
    }
    else {
        evil_twin->prox        = NULL;
        evil_twin->prox_boosts = NULL;
    }

    return evil_twin;
}

void
RichPost_read_record(RichPosting *self, InStream *instream)
{
    float *const norm_decoder = self->sim->norm_decoder;
    u32_t  doc_code;
    u32_t  num_prox = 0;
    u32_t  position = 0; 
    u32_t *positions;
    float *prox_boosts;
    float  aggregate_weight = 0.0;

    /* Decode delta doc. */
    doc_code = InStream_Read_C32(instream);
    self->doc_id   += doc_code >> 1;

    /* If the stored num was odd, the freq is 1. */ 
    if (doc_code & 1) {
        self->freq = 1;
    }
    /* Otherwise, freq was stored as a C32. */
    else {
        self->freq = InStream_Read_C32(instream);
    } 

    /* Read positions, aggregate per-position boost byte into weight. */
    num_prox = self->freq;
    if (num_prox > self->prox_cap) {
        self->prox        = REALLOCATE(self->prox, num_prox, u32_t);
        self->prox_boosts = REALLOCATE(self->prox_boosts, num_prox, float);
    }
    positions   = self->prox;
    prox_boosts = self->prox_boosts;

    while (num_prox--) {
        position += InStream_Read_C32(instream);
        *positions++ = position;
        *prox_boosts = norm_decoder[ InStream_Read_U8(instream) ];
        aggregate_weight += *prox_boosts;
        prox_boosts++;
    }
    self->weight = aggregate_weight / self->freq;
}

void
RichPost_add_inversion_to_pool(RichPosting *self, PostingPool *post_pool, 
                               Inversion *inversion, FieldType *type, 
                               i32_t doc_id, float doc_boost,
                               float length_norm)
{
    MemoryPool  *mem_pool = post_pool->mem_pool;
    Similarity  *sim = self->sim;
    float        field_boost = doc_boost * type->boost * length_norm;
    Token      **tokens;
    u32_t        freq;

    Inversion_Reset(inversion);
    while ( (tokens = Inversion_Next_Cluster(inversion, &freq)) != NULL ) {
        Token   *token          = *tokens;
        u32_t    raw_post_bytes = MAX_RAW_POSTING_LEN(token->len, freq);
        RawPosting *raw_posting = RawPost_new(
            MemPool_Grab(mem_pool, raw_post_bytes), doc_id, freq,
            token->text, token->len
        );
        char *const start = raw_posting->blob + token->len;
        char *dest        = start;
        u32_t last_prox   = 0;
        u32_t i;

        /* Positions and boosts. */
        for (i = 0; i < freq; i++) {
            Token *const t = tokens[i];
            const u32_t prox_delta = t->pos - last_prox;
            const float boost = field_boost * t->boost;

            Math_encode_c32(prox_delta, &dest);
            last_prox = t->pos; 

            *((u8_t*)dest) = Sim_Encode_Norm(sim, boost);
            dest++;
        }

        /* Resize raw posting memory allocation. */
        raw_posting->aux_len = dest - start;
        raw_post_bytes = dest - (char*)raw_posting;
        MemPool_Resize(mem_pool, raw_posting, raw_post_bytes);
        PostPool_Add_Elem(post_pool, (Obj*)raw_posting);
    }
}

RawPosting*
RichPost_read_raw(RichPosting *self, InStream *instream, i32_t last_doc_id, 
                  CharBuf *term_text, MemoryPool *mem_pool)
{
    const size_t text_size        = CB_Get_Size(term_text);
    const u32_t  doc_code         = InStream_Read_C32(instream);
    const u32_t  delta_doc        = doc_code >> 1;
    const i32_t  doc_id           = last_doc_id + delta_doc;
    const u32_t  freq             = (doc_code & 1) 
                                  ? 1 
                                  : InStream_Read_C32(instream);
    size_t raw_post_bytes         = MAX_RAW_POSTING_LEN(text_size, freq);
    void *const allocation        = MemPool_Grab(mem_pool, raw_post_bytes);
    RawPosting *const raw_posting = RawPost_new(allocation, doc_id, freq,
        term_text->ptr, text_size);
    u32_t  num_prox   = freq;
    char *const start = raw_posting->blob + text_size;
    char *      dest  = start;
    UNUSED_VAR(self);

    /* Read positions and per-position boosts. */
    while (num_prox--) {
        dest += InStream_Read_Raw_C64(instream, dest);
        *((u8_t*)dest) = InStream_Read_U8(instream);
        dest++;
    }

    /* Resize raw posting memory allocation. */
    raw_posting->aux_len = dest - start;
    raw_post_bytes       = dest - (char*)raw_posting;
    MemPool_Resize(mem_pool, raw_posting, raw_post_bytes);

    return raw_posting;
}

RichPostingScorer*
RichPost_make_matcher(RichPosting *self, Similarity *sim, 
                      PostingList *plist, Compiler *compiler,
                      bool_t need_score)
{
    RichPostingScorer* matcher
        = (RichPostingScorer*)VTable_Make_Obj(&RICHPOSTINGSCORER);
    UNUSED_VAR(self);
    UNUSED_VAR(need_score);
    return RichPostScorer_init(matcher, sim, plist, compiler);
}

RichPostingScorer*
RichPostScorer_init(RichPostingScorer *self, Similarity *sim, 
                    PostingList *plist, Compiler *compiler)
{
    return (RichPostingScorer*)ScorePostScorer_init(
        (ScorePostingScorer*)self, sim, plist, compiler);
}

/* Copyright 2007-2009 Marvin Humphrey
 *
 * This program is free software; you can redistribute it and/or modify
 * under the same terms as Perl itself.
 */

