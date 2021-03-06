#include "Search/Kino03/KinoSearch/Util/ToolSet.h"

#include "Search/Kino03/KinoSearch/Search/ORQuery.h"
#include "Search/Kino03/KinoSearch/Index/SegReader.h"
#include "Search/Kino03/KinoSearch/Search/ORMatcher.h"
#include "Search/Kino03/KinoSearch/Search/Searchable.h"
#include "Search/Kino03/KinoSearch/Search/Similarity.h"
#include "Search/Kino03/KinoSearch/Store/InStream.h"
#include "Search/Kino03/KinoSearch/Store/OutStream.h"

ORQuery*
ORQuery_new(VArray *children)
{
    ORQuery *self = (ORQuery*)VTable_Make_Obj(&ORQUERY);
    return ORQuery_init(self, children);
}

ORQuery*
ORQuery_init(ORQuery *self, VArray *children)
{
    return (ORQuery*)PolyQuery_init((PolyQuery*)self, children);
}

Compiler*
ORQuery_make_compiler(ORQuery *self, Searchable *searchable, float boost)
{
    return (Compiler*)ORCompiler_new(self, searchable, boost);
}

bool_t
ORQuery_equals(ORQuery *self, Obj *other)
{
    if ((ORQuery*)other == self)   { return true;  }
    if (!OBJ_IS_A(other, ORQUERY)) { return false; }
    return PolyQuery_equals((PolyQuery*)self, other);
}

CharBuf*
ORQuery_to_string(ORQuery *self)
{
    u32_t num_kids = VA_Get_Size(self->children);
    if (!num_kids) return CB_new_from_trusted_utf8("()", 2);
    else {
        CharBuf *retval = CB_new_from_trusted_utf8("(", 1);
        u32_t i;
        u32_t last_kid = num_kids - 1;
        for (i = 0; i < num_kids; i++) {
            CharBuf *kid_string = Obj_To_String(VA_Fetch(self->children, i));
            CB_Cat(retval, kid_string);
            DECREF(kid_string);
            if (i == last_kid) {
                CB_Cat_Trusted_Str(retval, ")", 1);
            }
            else {
                CB_Cat_Trusted_Str(retval, " OR ", 4);
            }
        }
        return retval;
    }
}

/**********************************************************************/

ORCompiler*
ORCompiler_new(ORQuery *parent, Searchable *searchable, float boost)
{
    ORCompiler *self = (ORCompiler*)VTable_Make_Obj(&ORCOMPILER);
    return ORCompiler_init(self, parent, searchable, boost);
}

ORCompiler*
ORCompiler_init(ORCompiler *self, ORQuery *parent, Searchable *searchable, 
                 float boost)
{
    PolyCompiler_init((PolyCompiler*)self, (PolyQuery*)parent, searchable, 
        boost);
    Compiler_Normalize(self);
    return self;
}

Matcher*
ORCompiler_make_matcher(ORCompiler *self, SegReader *reader, 
                        bool_t need_score)
{
    u32_t num_kids = VA_Get_Size(self->children);

    if (num_kids == 1) {
        Compiler *only_child = (Compiler*)VA_Fetch(self->children, 0);
        return Compiler_Make_Matcher(only_child, reader, need_score);
    }
    else {
        VArray *submatchers = VA_new(num_kids);
        u32_t i;
        u32_t num_submatchers = 0;

        /* Accumulate sub-matchers. */
        for (i = 0; i < num_kids; i++) {
            Compiler *child = (Compiler*)VA_Fetch(self->children, i);
            Matcher *submatcher 
                = Compiler_Make_Matcher(child, reader, need_score);
            if (submatcher != NULL) {
                VA_Push(submatchers, (Obj*)submatcher);
                num_submatchers++;
            }
        }

        if (num_submatchers == 0) {
            /* No possible matches, so return null. */
            DECREF(submatchers);
            return NULL;
        }
        else if (num_submatchers == 1) {
            /* Only one submatcher, so no need for ORScorer wrapper. */
            Matcher *submatcher = (Matcher*)INCREF(VA_Fetch(submatchers, 0));
            DECREF(submatchers);
            return submatcher;
        }
        else {
            Similarity *sim    = Compiler_Get_Similarity(self);
            Matcher    *retval = need_score
                ? (Matcher*)ORScorer_new(submatchers, sim)
                : (Matcher*)ORMatcher_new(submatchers);
            DECREF(submatchers);
            return retval;
        }
    }
}

/* Copyright 2006-2009 Marvin Humphrey
 *
 * This program is free software; you can redistribute it and/or modify
 * under the same terms as Perl itself.
 */

