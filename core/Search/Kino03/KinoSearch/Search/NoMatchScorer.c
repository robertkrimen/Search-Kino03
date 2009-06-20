#include "Search/Kino03/KinoSearch/Util/ToolSet.h"

#include "Search/Kino03/KinoSearch/Search/NoMatchScorer.h"
#include "Search/Kino03/KinoSearch/Schema.h"
#include "Search/Kino03/KinoSearch/Index/IndexReader.h"
#include "Search/Kino03/KinoSearch/Search/Similarity.h"

NoMatchScorer*
NoMatchScorer_new()
{
    NoMatchScorer *self = (NoMatchScorer*)VTable_Make_Obj(&NOMATCHSCORER);
    return NoMatchScorer_init(self);
}

NoMatchScorer*
NoMatchScorer_init(NoMatchScorer *self)
{
    return (NoMatchScorer*)Matcher_init((Matcher*)self);
}   

i32_t
NoMatchScorer_next(NoMatchScorer* self) 
{
    UNUSED_VAR(self);
    return 0;
}

i32_t
NoMatchScorer_advance(NoMatchScorer* self, i32_t target) 
{
    UNUSED_VAR(self);
    UNUSED_VAR(target);
    return 0;
}

/* Copyright 2007-2009 Marvin Humphrey
 *
 * This program is free software; you can redistribute it and/or modify
 * under the same terms as Perl itself.
 */

