#include "Search/Kino03/KinoSearch/Util/ToolSet.h"

#include "Search/Kino03/KinoSearch/Search/MatchAllQuery.h"
#include "Search/Kino03/KinoSearch/Schema.h"
#include "Search/Kino03/KinoSearch/Search/Span.h"
#include "Search/Kino03/KinoSearch/Index/DocVector.h"
#include "Search/Kino03/KinoSearch/Index/SegReader.h"
#include "Search/Kino03/KinoSearch/Search/MatchAllScorer.h"
#include "Search/Kino03/KinoSearch/Search/Searchable.h"
#include "Search/Kino03/KinoSearch/Search/Similarity.h"
#include "Search/Kino03/KinoSearch/Store/InStream.h"
#include "Search/Kino03/KinoSearch/Store/OutStream.h"
#include "Search/Kino03/KinoSearch/Util/Freezer.h"

MatchAllQuery*
MatchAllQuery_new()
{
    MatchAllQuery *self = (MatchAllQuery*)VTable_Make_Obj(&MATCHALLQUERY);
    return MatchAllQuery_init(self);
}

MatchAllQuery*
MatchAllQuery_init(MatchAllQuery *self)
{
    return (MatchAllQuery*)Query_init((Query*)self, 0.0f);
}

bool_t
MatchAllQuery_equals(MatchAllQuery *self, Obj *other)
{
    MatchAllQuery *evil_twin = (MatchAllQuery*)other;
    if (!OBJ_IS_A(evil_twin, MATCHALLQUERY)) return false;
    if (self->boost != evil_twin->boost) return false;
    return true;
}

CharBuf*
MatchAllQuery_to_string(MatchAllQuery *self)
{
    UNUSED_VAR(self);
    return CB_new_from_trusted_utf8("[MATCHALL]", 10);
}

Compiler*
MatchAllQuery_make_compiler(MatchAllQuery *self, Searchable *searchable, 
                            float boost)
{
    return (Compiler*)MatchAllCompiler_new(self, searchable, boost);
}

/**********************************************************************/

MatchAllCompiler*
MatchAllCompiler_new(MatchAllQuery *parent, Searchable *searchable, 
                     float boost)
{
    MatchAllCompiler *self 
        = (MatchAllCompiler*)VTable_Make_Obj(&MATCHALLCOMPILER);
    return MatchAllCompiler_init(self, parent, searchable, boost);
}

MatchAllCompiler*
MatchAllCompiler_init(MatchAllCompiler *self, MatchAllQuery *parent, 
                      Searchable *searchable, float boost)
{
    return (MatchAllCompiler*)Compiler_init((Compiler*)self, 
        (Query*)parent, searchable, NULL, boost);
}

MatchAllCompiler*
MatchAllCompiler_deserialize(MatchAllCompiler *self, InStream *instream)
{
    self = self ? self : (MatchAllCompiler*)VTable_Make_Obj(&MATCHALLCOMPILER);
    return (MatchAllCompiler*)Compiler_deserialize((Compiler*)self, instream);
}

Matcher*
MatchAllCompiler_make_matcher(MatchAllCompiler *self, SegReader *reader,
                              bool_t need_score)
{
    float weight = MatchAllCompiler_Get_Weight(self);
    UNUSED_VAR(need_score);
    return (Matcher*)MatchAllScorer_new(weight, SegReader_Doc_Max(reader));
}

/* Copyright 2006-2009 Marvin Humphrey
 *
 * This program is free software; you can redistribute it and/or modify
 * under the same terms as Perl itself.
 */

