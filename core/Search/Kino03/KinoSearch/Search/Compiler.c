#include "Search/Kino03/KinoSearch/Util/ToolSet.h"

#include "Search/Kino03/KinoSearch/Search/Compiler.h"
#include "Search/Kino03/KinoSearch/Index/SegReader.h"
#include "Search/Kino03/KinoSearch/Index/DocVector.h"
#include "Search/Kino03/KinoSearch/Schema.h"
#include "Search/Kino03/KinoSearch/Search/Matcher.h"
#include "Search/Kino03/KinoSearch/Search/Query.h"
#include "Search/Kino03/KinoSearch/Search/Searchable.h"
#include "Search/Kino03/KinoSearch/Search/Similarity.h"
#include "Search/Kino03/KinoSearch/Store/InStream.h"
#include "Search/Kino03/KinoSearch/Store/OutStream.h"
#include "Search/Kino03/KinoSearch/Util/Freezer.h"

Compiler*
Compiler_init(Compiler *self, Query *parent, Searchable *searchable,
            Similarity *sim, float boost)
{
    Query_init((Query*)self, boost);
    if (!sim) {
        Schema *schema = Searchable_Get_Schema(searchable);
        sim = Schema_Get_Similarity(schema);
    }
    self->parent  = (Query*)INCREF(parent);
    self->sim     = (Similarity*)INCREF(sim);
    ABSTRACT_CLASS_CHECK(self, COMPILER);
    return self;
}

void
Compiler_destroy(Compiler *self)
{
    DECREF(self->parent);
    DECREF(self->sim);
    SUPER_DESTROY(self, COMPILER);
}

float
Compiler_get_weight(Compiler *self)
{
    return Compiler_Get_Boost(self);
}

Similarity*
Compiler_get_similarity(Compiler *self) { return self->sim; }
Query*
Compiler_get_parent(Compiler *self)     { return self->parent; }

float
Compiler_sum_of_squared_weights(Compiler *self)
{
    UNUSED_VAR(self);
    return 1.0f;
}

void
Compiler_apply_norm_factor(Compiler *self, float factor)
{
    UNUSED_VAR(self);
    UNUSED_VAR(factor);
}

void
Compiler_normalize(Compiler *self)
{
    /* factor = ( tf_q * idf_t ) */
    float factor = Compiler_Sum_Of_Squared_Weights(self); 

    /* factor /= norm_q */
    factor = Sim_Query_Norm(self->sim, factor);

    /* weight *= factor */
    Compiler_Apply_Norm_Factor(self, factor); 
}

VArray*
Compiler_highlight_spans(Compiler *self, Searchable *searchable, 
                       DocVector *doc_vec, const CharBuf *field)
{
    UNUSED_VAR(self);
    UNUSED_VAR(searchable);
    UNUSED_VAR(doc_vec);
    UNUSED_VAR(field);
    return VA_new(0);
}

CharBuf*
Compiler_to_string(Compiler *self)
{
    CharBuf *stringified_query = Query_To_String(self->parent);
    CharBuf *string = CB_new_from_trusted_utf8("compiler(", 9);
    CB_Cat(string, stringified_query);
    CB_Cat_Trusted_Str(string, ")", 1);
    DECREF(stringified_query);
    return string;
}

bool_t
Compiler_equals(Compiler *self, Obj *other)
{
    Compiler *evil_twin = (Compiler*)other;
    if (evil_twin == self) return true;
    if (!OBJ_IS_A(evil_twin, COMPILER)) return false;
    if (self->boost != evil_twin->boost) return false;
    if (!Query_Equals(self->parent, (Obj*)evil_twin->parent)) return false;
    if (!Sim_Equals(self->sim, (Obj*)evil_twin->sim)) return false;
    return true;
}

void
Compiler_serialize(Compiler *self, OutStream *outstream)
{
    ABSTRACT_CLASS_CHECK(self, COMPILER);
    OutStream_Write_Float(outstream, self->boost);
    FREEZE(self->parent, outstream);
    FREEZE(self->sim, outstream);
}

Compiler*
Compiler_deserialize(Compiler *self, InStream *instream)
{
    if (!self) THROW("Compiler_Deserialize is abstract");
    self->boost  = InStream_Read_Float(instream);
    self->parent = (Query*)THAW(instream);
    self->sim    = (Similarity*)THAW(instream);
    return self;
}

/* Copyright 2006-2009 Marvin Humphrey
 *
 * This program is free software; you can redistribute it and/or modify
 * under the same terms as Perl itself.
 */

