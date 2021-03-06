#include "Search/Kino03/KinoSearch/Util/ToolSet.h"

#include "Search/Kino03/KinoSearch/Search/NOTQuery.h"
#include "Search/Kino03/KinoSearch/Index/DocVector.h"
#include "Search/Kino03/KinoSearch/Index/SegReader.h"
#include "Search/Kino03/KinoSearch/Search/MatchAllScorer.h"
#include "Search/Kino03/KinoSearch/Search/NOTScorer.h"
#include "Search/Kino03/KinoSearch/Search/Searchable.h"
#include "Search/Kino03/KinoSearch/Store/InStream.h"
#include "Search/Kino03/KinoSearch/Store/OutStream.h"

NOTQuery*
NOTQuery_new(Query *negated_query)
{
    NOTQuery *self = (NOTQuery*)VTable_Make_Obj(&NOTQUERY);
    return NOTQuery_init(self, negated_query);
}

NOTQuery*
NOTQuery_init(NOTQuery *self, Query *negated_query)
{
    self = (NOTQuery*)PolyQuery_init((PolyQuery*)self, NULL);
    NOTQuery_Set_Boost(self, 0.0f);
    PolyQuery_add_child((PolyQuery*)self, negated_query);
    return self;
}

Query*
NOTQuery_get_negated_query(NOTQuery *self) 
{ 
    return (Query*)VA_Fetch(self->children, 0);
}

void
NOTQuery_set_negated_query(NOTQuery *self, Query *negated_query)
{
    VA_Store(self->children, 0, INCREF(negated_query));
}

CharBuf*
NOTQuery_to_string(NOTQuery *self)
{
    CharBuf *neg_string = Obj_To_String(VA_Fetch(self->children, 0));
    CharBuf *retval = CB_newf("-%o", neg_string);
    DECREF(neg_string);
    return retval;
}

bool_t
NOTQuery_equals(NOTQuery *self, Obj *other)
{
    if ((NOTQuery*)other == self)   { return true; }
    if (!OBJ_IS_A(other, NOTQUERY)) { return false; }
    return PolyQuery_equals((PolyQuery*)self, other);
}

Compiler*
NOTQuery_make_compiler(NOTQuery *self, Searchable *searchable, float boost)
{
    return (Compiler*)NOTCompiler_new(self, searchable, boost);
}

/**********************************************************************/

NOTCompiler*
NOTCompiler_new(NOTQuery *parent, Searchable *searchable, float boost)
{
    NOTCompiler *self = (NOTCompiler*)VTable_Make_Obj(&NOTCOMPILER);
    return NOTCompiler_init(self, parent, searchable, boost);
}

NOTCompiler*
NOTCompiler_init(NOTCompiler *self, NOTQuery *parent, Searchable *searchable, 
                 float boost)
{
    PolyCompiler_init((PolyCompiler*)self, (PolyQuery*)parent, searchable, 
        boost);
    Compiler_Normalize(self);
    return self;
}

float
NOTCompiler_sum_of_squared_weights(NOTCompiler *self)
{
    UNUSED_VAR(self);
    return 0.0f;
}

VArray*
NOTCompiler_highlight_spans(NOTCompiler *self, Searchable *searchable, 
                            DocVector *doc_vec, const CharBuf *field)
{
    UNUSED_VAR(self);
    UNUSED_VAR(searchable);
    UNUSED_VAR(doc_vec);
    UNUSED_VAR(field);
    return VA_new(0);
}

Matcher*
NOTCompiler_make_matcher(NOTCompiler *self, SegReader *reader, 
                         bool_t need_score)
{
    Matcher *negated_matcher 
        = Compiler_Make_Matcher(VA_Fetch(self->children, 0), reader, false);
    UNUSED_VAR(need_score);

    if (negated_matcher == NULL) {
        float weight = Compiler_Get_Weight(self);
        return (Matcher*)MatchAllScorer_new(weight, SegReader_Doc_Max(reader));
    }
    else if (OBJ_IS_A(negated_matcher, MATCHALLSCORER)) {
        DECREF(negated_matcher);
        return NULL;
    }
    else {
        Matcher *retval 
            = (Matcher*)NOTScorer_new(negated_matcher, SegReader_Doc_Max(reader));
        DECREF(negated_matcher);
        return retval;
    }
}

/* Copyright 2006-2009 Marvin Humphrey
 *
 * This program is free software; you can redistribute it and/or modify
 * under the same terms as Perl itself.
 */

