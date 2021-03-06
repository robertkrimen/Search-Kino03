#include "Search/Kino03/KinoSearch/Util/ToolSet.h"

#include "Search/Kino03/KinoSearch/Search/PolyQuery.h"
#include "Search/Kino03/KinoSearch/Schema.h"
#include "Search/Kino03/KinoSearch/Search/Span.h"
#include "Search/Kino03/KinoSearch/Index/DocVector.h"
#include "Search/Kino03/KinoSearch/Search/Searchable.h"
#include "Search/Kino03/KinoSearch/Search/Similarity.h"
#include "Search/Kino03/KinoSearch/Store/InStream.h"
#include "Search/Kino03/KinoSearch/Store/OutStream.h"
#include "Search/Kino03/KinoSearch/Util/Freezer.h"

PolyQuery*
PolyQuery_init(PolyQuery *self, VArray *children)
{
    u32_t i;
    const u32_t num_kids = children ? VA_Get_Size(children) : 0;
    Query_init((Query*)self, 1.0f);
    self->children = VA_new(num_kids);
    for (i = 0; i < num_kids; i++) {
        PolyQuery_Add_Child(self, (Query*)VA_Fetch(children, i));
    }
    return self;
}

void
PolyQuery_destroy(PolyQuery *self)
{
    DECREF(self->children);
    SUPER_DESTROY(self, POLYQUERY);
}

void
PolyQuery_add_child(PolyQuery *self, Query *query)
{
    ASSERT_IS_A(query, QUERY);
    VA_Push(self->children, INCREF(query));
}

void
PolyQuery_set_children(PolyQuery *self, VArray *children)
{
    DECREF(self->children);
    self->children = (VArray*)INCREF(children);
}

VArray*
PolyQuery_get_children(PolyQuery *self) { return self->children; }

void
PolyQuery_serialize(PolyQuery *self, OutStream *outstream)
{
    const u32_t num_kids =  VA_Get_Size(self->children);
    u32_t i;
    OutStream_Write_Float(outstream, self->boost);
    OutStream_Write_U32(outstream, num_kids);
    for (i = 0; i < num_kids; i++) {
        Query *child = (Query*)VA_Fetch(self->children, i);
        FREEZE(child, outstream);
    }
}

PolyQuery*
PolyQuery_deserialize(PolyQuery *self, InStream *instream)
{
    float boost          = InStream_Read_Float(instream);
    u32_t num_children   = InStream_Read_U32(instream);

    if (!self) THROW("Abstract class");
    PolyQuery_init(self, NULL);
    PolyQuery_Set_Boost(self, boost);

    VA_Grow(self->children, num_children);
    while (num_children--) {
        VA_Push(self->children, THAW(instream));
    }

    return self;
}

bool_t
PolyQuery_equals(PolyQuery *self, Obj *other)
{
    PolyQuery *evil_twin = (PolyQuery*)other;
    if (evil_twin == self) return true;
    if (!OBJ_IS_A(evil_twin, POLYQUERY)) return false;
    if (self->boost != evil_twin->boost) return false;
    if (!VA_Equals(evil_twin->children, (Obj*)self->children)) return false;
    return true;
}

/**********************************************************************/


PolyCompiler*
PolyCompiler_init(PolyCompiler *self, PolyQuery *parent, Searchable *searchable, 
                 float boost)
{
    u32_t i;
    const u32_t num_kids = VA_Get_Size(parent->children);

    Compiler_init((Compiler*)self, (Query*)parent, searchable, NULL, boost);
    self->children = VA_new(num_kids);

    /* Iterate over the children, creating a Compiler for each one. */
    for (i = 0; i < num_kids; i++) {
        Query *child_query = (Query*)VA_Fetch(parent->children, i);
        float sub_boost = boost * Query_Get_Boost(child_query);
        VA_Push(self->children, 
            (Obj*)Query_Make_Compiler(child_query, searchable, sub_boost));
    }

    return self;
}

void
PolyCompiler_destroy(PolyCompiler *self)
{
    DECREF(self->children);
    SUPER_DESTROY(self, POLYCOMPILER);
}

float
PolyCompiler_sum_of_squared_weights(PolyCompiler *self)
{
    float sum = 0;
    u32_t i, max;
    float my_boost = Compiler_Get_Boost(self);

    for (i = 0, max = VA_Get_Size(self->children); i < max; i++) {
        Compiler *child = (Compiler*)VA_Fetch(self->children, i);
        sum += Compiler_Sum_Of_Squared_Weights(child);
    }

    /* Compound the weight of each child. */
    sum *= my_boost * my_boost;

    return sum;
}

void
PolyCompiler_apply_norm_factor(PolyCompiler *self, float factor)
{
    u32_t i, max;
    for (i = 0, max = VA_Get_Size(self->children); i < max; i++) {
        Compiler *child = (Compiler*)VA_Fetch(self->children, i);
        Compiler_Apply_Norm_Factor(child, factor);
    }
}

VArray*
PolyCompiler_highlight_spans(PolyCompiler *self, Searchable *searchable, 
                            DocVector *doc_vec, const CharBuf *field)
{
    VArray *spans = VA_new(0);
    u32_t i, max;
    for (i = 0, max = VA_Get_Size(self->children); i < max; i++) {
        Query *child = (Query*)VA_Fetch(self->children, i);
        VArray *child_spans = Compiler_Highlight_Spans(child, searchable,
            doc_vec, field);
        if (child_spans) {
            VA_Push_VArray(spans, child_spans);
            VA_Dec_RefCount(child_spans);
        }
    }
    return spans;
}

void
PolyCompiler_serialize(PolyCompiler *self, OutStream *outstream)
{
    CB_Serialize(Obj_Get_Class_Name(self), outstream);
    VA_Serialize(self->children, outstream);
    Compiler_serialize((Compiler*)self, outstream);
}

PolyCompiler*
PolyCompiler_deserialize(PolyCompiler *self, InStream *instream)
{
    CharBuf *class_name = CB_deserialize(NULL, instream);
    if (!self) {
        VTable *vtable = (VTable*)VTable_singleton(class_name, NULL);
        self = (PolyCompiler*)VTable_Make_Obj(vtable);
    }
    DECREF(class_name);
    self->children = VA_deserialize(NULL, instream);
    return (PolyCompiler*)Compiler_deserialize((Compiler*)self, instream);
}

/* Copyright 2006-2009 Marvin Humphrey
 *
 * This program is free software; you can redistribute it and/or modify
 * under the same terms as Perl itself.
 */

