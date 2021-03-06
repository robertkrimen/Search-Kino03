#include "Search/Kino03/KinoSearch/Util/ToolSet.h"

#include "Search/Kino03/KinoSearch/Search/MatchAllScorer.h"

MatchAllScorer*
MatchAllScorer_new(float score, i32_t doc_max)
{
    MatchAllScorer *self 
        = (MatchAllScorer*)VTable_Make_Obj(&MATCHALLSCORER);
    return MatchAllScorer_init(self, score, doc_max);
}

MatchAllScorer*
MatchAllScorer_init(MatchAllScorer *self, float score, i32_t doc_max)
{
    Matcher_init((Matcher*)self);
    self->doc_id        = 0;
    self->score         = score;
    self->doc_max       = doc_max;
    return self;
}

i32_t
MatchAllScorer_next(MatchAllScorer* self) 
{
    if (++self->doc_id <= self->doc_max) {
        return self->doc_id;
    }
    else {
        self->doc_id--;
        return 0;
    }
}

i32_t
MatchAllScorer_advance(MatchAllScorer* self, i32_t target) 
{
    self->doc_id = target - 1;
    return MatchAllScorer_next(self);
}

float
MatchAllScorer_score(MatchAllScorer* self) 
{
    return self->score;
}

i32_t 
MatchAllScorer_get_doc_id(MatchAllScorer* self) 
{
    return self->doc_id;
}

/* Copyright 2007-2009 Marvin Humphrey
 *
 * This program is free software; you can redistribute it and/or modify
 * under the same terms as Perl itself.
 */

