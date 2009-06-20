#include <string.h>

#include "Search/Kino03/KinoSearch/Util/ToolSet.h"

#include "Search/Kino03/KinoSearch/Index/PostingList.h"
#include "Search/Kino03/KinoSearch/Posting.h"
#include "Search/Kino03/KinoSearch/Index/Lexicon.h"
#include "Search/Kino03/KinoSearch/Util/Err.h"
#include "Search/Kino03/KinoSearch/Util/MemManager.h"

PostingList*
PList_init(PostingList *self)
{
    ABSTRACT_CLASS_CHECK(self, POSTINGLIST);
    return self;
}

i32_t
PList_advance(PostingList *self, i32_t target) 
{
    while (1) {
        i32_t doc_id = PList_Next(self);
        if (doc_id == 0 || doc_id >= target)
            return doc_id; 
    }
}

/* Copyright 2007-2009 Marvin Humphrey
 *
 * This program is free software; you can redistribute it and/or modify
 * under the same terms as Perl itself.
 */

