#include <stdarg.h>

#include "Search/Kino03/KinoSearch/Util/ToolSet.h"

#include "Search/Kino03/KinoSearch/Search/PhraseQuery.h"
#include "Search/Kino03/KinoSearch/Posting.h"
#include "Search/Kino03/KinoSearch/Posting/ScorePosting.h"
#include "Search/Kino03/KinoSearch/Schema.h"
#include "Search/Kino03/KinoSearch/Search/Span.h"
#include "Search/Kino03/KinoSearch/Index/DocVector.h"
#include "Search/Kino03/KinoSearch/Index/SegReader.h"
#include "Search/Kino03/KinoSearch/Index/PostingList.h"
#include "Search/Kino03/KinoSearch/Index/PostingsReader.h"
#include "Search/Kino03/KinoSearch/Index/SegPostingList.h"
#include "Search/Kino03/KinoSearch/Index/TermVector.h"
#include "Search/Kino03/KinoSearch/Search/PhraseScorer.h"
#include "Search/Kino03/KinoSearch/Search/Searchable.h"
#include "Search/Kino03/KinoSearch/Search/Similarity.h"
#include "Search/Kino03/KinoSearch/Search/TermQuery.h"
#include "Search/Kino03/KinoSearch/Store/InStream.h"
#include "Search/Kino03/KinoSearch/Store/OutStream.h"
#include "Search/Kino03/KinoSearch/Util/BitVector.h"
#include "Search/Kino03/KinoSearch/Util/Freezer.h"
#include "Search/Kino03/KinoSearch/Util/I32Array.h"

/* Shared initialization routine which assumes that it's ok to assume control
 * over [field] and [terms], eating their refcounts. */
static PhraseQuery*
S_do_init(PhraseQuery *self, CharBuf *field, VArray *terms, float boost);

PhraseQuery*
PhraseQuery_new(const CharBuf *field, VArray *terms)
{
    PhraseQuery *self = (PhraseQuery*)VTable_Make_Obj(&PHRASEQUERY);
    return PhraseQuery_init(self, field, terms);
}

PhraseQuery*
PhraseQuery_init(PhraseQuery *self, const CharBuf *field, VArray *terms)
{
    return S_do_init(self, CB_Clone(field), VA_Clone(terms), 1.0f);
}

void
PhraseQuery_destroy(PhraseQuery *self)
{
    DECREF(self->terms);
    DECREF(self->field);
    SUPER_DESTROY(self, PHRASEQUERY);
}

static PhraseQuery*
S_do_init(PhraseQuery *self, CharBuf *field, VArray *terms, float boost)
{
    u32_t i, max;
    Query_init((Query*)self, boost);
    for (i = 0, max = VA_Get_Size(terms); i < max; i++) {
        ASSERT_IS_A(VA_Fetch(terms, i), OBJ);
    }
    self->field = field;
    self->terms = terms;
    return self;
}

void
PhraseQuery_serialize(PhraseQuery *self, OutStream *outstream)
{
    OutStream_Write_Float(outstream, self->boost);
    CB_Serialize(self->field, outstream);
    VA_Serialize(self->terms, outstream);
}

PhraseQuery*
PhraseQuery_deserialize(PhraseQuery *self, InStream *instream)
{
    float    boost  = InStream_Read_Float(instream);
    CharBuf *field  = CB_deserialize(NULL, instream);
    VArray  *terms  = VA_deserialize(NULL, instream);
    self = self ? self : (PhraseQuery*)VTable_Make_Obj(&PHRASEQUERY);
    return S_do_init(self, field, terms, boost);
}

bool_t
PhraseQuery_equals(PhraseQuery *self, Obj *other)
{
    PhraseQuery *evil_twin = (PhraseQuery*)other;
    if (evil_twin == self) return true;
    if (!OBJ_IS_A(evil_twin, PHRASEQUERY)) return false;
    if (self->boost != evil_twin->boost) return false;
    if (self->field && !evil_twin->field) return false;
    if (!self->field && evil_twin->field) return false;
    if (self->field && !CB_Equals(self->field, (Obj*)evil_twin->field)) 
        return false;
    if (!VA_Equals(evil_twin->terms, (Obj*)self->terms)) return false;
    return true;
}

CharBuf*
PhraseQuery_to_string(PhraseQuery *self)
{
    u32_t i;
    u32_t num_terms = VA_Get_Size(self->terms);
    CharBuf *retval = CB_Clone(self->field);
    CB_Cat_Trusted_Str(retval, ":\"", 2);
    for (i = 0; i < num_terms; i++) {
        Obj *term = VA_Fetch(self->terms, i);
        CharBuf *term_string = Obj_To_String(term);
        CB_Cat(retval, term_string);
        DECREF(term_string);
        if (i < num_terms - 1) {
            CB_Cat_Trusted_Str(retval, " ",  1);
        }
    }
    CB_Cat_Trusted_Str(retval, "\"", 1);
    return retval;
}

Compiler*
PhraseQuery_make_compiler(PhraseQuery *self, Searchable *searchable, 
                          float boost)
{
    if (VA_Get_Size(self->terms) == 1) {
        /* Optimize for one-term "phrases". */
        Obj *term = VA_Fetch(self->terms, 0);
        TermQuery *term_query = TermQuery_new(self->field, term);
        TermCompiler *term_compiler;
        TermQuery_Set_Boost(term_query, self->boost);
        term_compiler = (TermCompiler*)TermQuery_Make_Compiler(term_query,
            searchable, boost);
        DECREF(term_query);
        return (Compiler*)term_compiler;
    }
    else {
        return (Compiler*)PhraseCompiler_new(self, searchable, boost);
    }
}

CharBuf*
PhraseQuery_get_field(PhraseQuery *self) { return self->field; }
VArray*
PhraseQuery_get_terms(PhraseQuery *self) { return self->terms; }

/*********************************************************************/

PhraseCompiler*
PhraseCompiler_new(PhraseQuery *parent, Searchable *searchable, float boost)
{
    PhraseCompiler *self = (PhraseCompiler*)VTable_Make_Obj(&PHRASECOMPILER);
    return PhraseCompiler_init(self, parent, searchable, boost);
}

PhraseCompiler*
PhraseCompiler_init(PhraseCompiler *self, PhraseQuery *parent, 
                    Searchable *searchable, float boost)
{
    Schema     *schema = Searchable_Get_Schema(searchable);
    Similarity *sim    = Schema_Fetch_Sim(schema, parent->field);
    VArray     *terms  = parent->terms;
    u32_t i, max;

    /* Try harder to find a Similarity if necessary. */
    if (!sim) { sim = Schema_Get_Similarity(schema); }

    /* Init. */
    Compiler_init((Compiler*)self, (Query*)parent, searchable, sim, boost);

    /* Store IDF for the phrase. */
    self->idf = 0;
    for (i = 0, max = VA_Get_Size(terms); i < max; i++) {
        Obj *term = VA_Fetch(terms, i);
        self->idf += Sim_IDF(sim, searchable, parent->field, term);
    }

    /* Calculate raw weight. */
    self->raw_weight = self->idf * self->boost;

    /* Make final preparations. */
    PhraseCompiler_Normalize(self);

    return self;
}

void
PhraseCompiler_serialize(PhraseCompiler *self, OutStream *outstream)
{
    Compiler_serialize((Compiler*)self, outstream);
    OutStream_Write_Float(outstream, self->idf);
    OutStream_Write_Float(outstream, self->raw_weight);
    OutStream_Write_Float(outstream, self->query_norm_factor);
    OutStream_Write_Float(outstream, self->normalized_weight);
}

PhraseCompiler*
PhraseCompiler_deserialize(PhraseCompiler *self, InStream *instream)
{
    self = self ? self : (PhraseCompiler*)VTable_Make_Obj(&PHRASECOMPILER);
    Compiler_deserialize((Compiler*)self, instream);
    self->idf               = InStream_Read_Float(instream);
    self->raw_weight        = InStream_Read_Float(instream);
    self->query_norm_factor = InStream_Read_Float(instream);
    self->normalized_weight = InStream_Read_Float(instream);
    return self;
}

bool_t
PhraseCompiler_equals(PhraseCompiler *self, Obj *other)
{
    PhraseCompiler *evil_twin = (PhraseCompiler*)other;
    if (!OBJ_IS_A(evil_twin, PHRASECOMPILER)) return false;
    if (!Compiler_equals((Compiler*)self, other)) return false;
    if (self->idf != evil_twin->idf) return false;
    if (self->raw_weight != evil_twin->raw_weight) return false;
    if (self->query_norm_factor != evil_twin->query_norm_factor) return false;
    if (self->normalized_weight != evil_twin->normalized_weight) return false;
    return true;
}

float
PhraseCompiler_get_weight(PhraseCompiler *self)
{
    return self->normalized_weight;
}

float
PhraseCompiler_sum_of_squared_weights(PhraseCompiler *self)
{
    return self->raw_weight * self->raw_weight;
}

void
PhraseCompiler_apply_norm_factor(PhraseCompiler *self, float factor)
{
    self->query_norm_factor = factor;
    self->normalized_weight = self->raw_weight * self->idf * factor;
}

Matcher*
PhraseCompiler_make_matcher(PhraseCompiler *self, SegReader *reader,
                            bool_t need_score)
{
    PostingsReader *const post_reader = (PostingsReader*)SegReader_Fetch(
        reader, POSTINGSREADER.name);
    PhraseQuery *const parent = (PhraseQuery*)self->parent;
    VArray  *const terms      = parent->terms;
    u32_t    num_terms        = VA_Get_Size(terms);
    Schema  *schema           = SegReader_Get_Schema(reader);
    Posting *posting          = Schema_Fetch_Posting(schema, parent->field);
    VArray  *plists;
    Matcher *retval;
    u32_t i;
    UNUSED_VAR(need_score);

    /* Bail if there are no terms. */
    if (!num_terms) return NULL;

    /* Bail unless field is valid and posting type supports positions. */
    if (posting == NULL || !OBJ_IS_A(posting, SCOREPOSTING)) return NULL;

    /* Bail if there's no PostingsReader for this segment. */
    if (!post_reader) { return NULL; }

    /* Look up each term. */
    plists = VA_new(num_terms);
    for (i = 0; i < num_terms; i++) {
        Obj *term = VA_Fetch(terms, i);
        PostingList *plist 
            = PostReader_Posting_List(post_reader, parent->field, term);

        /* Bail if any one of the terms isn't in the index. */
        if (!plist || !PList_Get_Doc_Freq(plist)) {
            DECREF(plist);
            DECREF(plists);
            return NULL;
        }
        VA_Push(plists, (Obj*)plist);
    }

    retval = (Matcher*)PhraseScorer_new(
        Compiler_Get_Similarity(self),
        plists, 
        (Compiler*)self
    );
    DECREF(plists);

    return retval;
}

VArray*
PhraseCompiler_highlight_spans(PhraseCompiler *self, Searchable *searchable, 
                               DocVector *doc_vec, const CharBuf *field)
{
    PhraseQuery *const parent = (PhraseQuery*)self->parent;
    VArray      *const terms  = parent->terms;
    VArray      *const spans  = VA_new(0);
    VArray      *term_vectors;
    BitVector   *posit_vec;
    BitVector   *other_posit_vec;
    u32_t        i;
    const u32_t  num_terms = VA_Get_Size(terms);
    u32_t        num_tvs;
    UNUSED_VAR(searchable);

    /* Bail if no terms or field doesn't match. */
    if (!num_terms) { return spans; }
    if (!CB_Equals(field, (Obj*)parent->field)) { return spans; }

    term_vectors    = VA_new(num_terms);
    posit_vec       = BitVec_new(0);
    other_posit_vec = BitVec_new(0);
    for (i = 0; i < num_terms; i++) {
        Obj *term = VA_Fetch(terms, i);
        TermVector *term_vector 
            = DocVec_Term_Vector(doc_vec, field, (CharBuf*)term);

        /* Bail if any term is missing. */
        if (!term_vector)
            break;

        VA_Push(term_vectors, (Obj*)term_vector);

        if (i == 0) {
            /* Set initial positions from first term. */
            u32_t j;
            I32Array *positions = term_vector->positions;
            for (j = I32Arr_Get_Size(positions); j > 0; j--) {
                BitVec_Set(posit_vec, I32Arr_Get(positions, j - 1));
            }
        }
        else {
            /* Filter positions using logical "and". */
            u32_t j;
            I32Array *positions = term_vector->positions;

            BitVec_Clear_All(other_posit_vec);
            for (j = I32Arr_Get_Size(positions); j > 0; j--) {
                i32_t pos = I32Arr_Get(positions, j - 1) - i;
                if (pos >= 0) {
                    BitVec_Set(other_posit_vec, pos);
                }
            }
            BitVec_And(posit_vec, other_posit_vec);
        }
    }

    /* Proceed only if all terms are present. */
    num_tvs = VA_Get_Size(term_vectors);
    if (num_tvs == num_terms) {
        TermVector *first_tv = (TermVector*)VA_Fetch(term_vectors, 0);
        TermVector *last_tv  = (TermVector*)VA_Fetch(term_vectors, num_tvs - 1);
        I32Array *tv_start_positions = first_tv->positions;
        I32Array *tv_end_positions   = last_tv->positions;
        I32Array *tv_start_offsets   = first_tv->start_offsets;
        I32Array *tv_end_offsets     = last_tv->end_offsets;
        u32_t     terms_max          = num_terms - 1;
        I32Array *valid_posits       = BitVec_To_Array(posit_vec);
        u32_t     num_valid_posits   = I32Arr_Get_Size(valid_posits);
        u32_t j = 0;
        u32_t posit_tick;
        float weight = Compiler_Get_Weight(self);
        i = 0;

        /* Add only those starts/ends that belong to a valid position. */
        for (posit_tick = 0; posit_tick < num_valid_posits; posit_tick++) {
            i32_t valid_start_posit = I32Arr_Get(valid_posits, posit_tick);
            i32_t valid_end_posit   = valid_start_posit + terms_max;
            i32_t start_offset = 0, end_offset = 0;
            u32_t max;

            for (max = I32Arr_Get_Size(tv_start_positions); i < max; i++) {
                if (I32Arr_Get(tv_start_positions, i) == valid_start_posit) {
                    start_offset = I32Arr_Get(tv_start_offsets, i);
                    break;
                }
            }
            for (max = I32Arr_Get_Size(tv_end_positions); j < max; j++) {
                if (I32Arr_Get(tv_end_positions, j) == valid_end_posit) {
                    end_offset = I32Arr_Get(tv_end_offsets, j);
                    break;
                }
            }

            VA_Push(spans, (Obj*)Span_new(start_offset, 
                end_offset - start_offset, weight) );

            i++, j++;
        }

        DECREF(valid_posits);
    }

    DECREF(other_posit_vec);
    DECREF(posit_vec);
    DECREF(term_vectors);
    return spans;
}

/* Copyright 2006-2009 Marvin Humphrey
 *
 * This program is free software; you can redistribute it and/or modify
 * under the same terms as Perl itself.
 */

