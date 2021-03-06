#include "Search/Kino03/KinoSearch/Util/ToolSet.h"

#include "Search/Kino03/KinoSearch/Search/TopDocs.h"
#include "Search/Kino03/KinoSearch/Index/IndexReader.h"
#include "Search/Kino03/KinoSearch/Index/Lexicon.h"
#include "Search/Kino03/KinoSearch/Search/SortRule.h"
#include "Search/Kino03/KinoSearch/Search/SortSpec.h"
#include "Search/Kino03/KinoSearch/Store/InStream.h"
#include "Search/Kino03/KinoSearch/Store/OutStream.h"
#include "Search/Kino03/KinoSearch/Util/I32Array.h"

TopDocs*
TopDocs_new(VArray *match_docs, u32_t total_hits)
{
    TopDocs *self = (TopDocs*)VTable_Make_Obj(&TOPDOCS);
    return TopDocs_init(self, match_docs, total_hits);
}

TopDocs*
TopDocs_init(TopDocs *self, VArray *match_docs, u32_t total_hits)
{
    self->match_docs = (VArray*)INCREF(match_docs);
    self->total_hits = total_hits;
    return self;
}

void
TopDocs_destroy(TopDocs *self)
{
    DECREF(self->match_docs);
    FREE_OBJ(self);
}

void
TopDocs_serialize(TopDocs *self, OutStream *outstream)
{
    VA_Serialize(self->match_docs, outstream);
    OutStream_Write_C32(outstream, self->total_hits);
}

TopDocs*
TopDocs_deserialize(TopDocs *self, InStream *instream)
{
    self = self ? self : (TopDocs*)VTable_Make_Obj(&TOPDOCS);
    self->match_docs = VA_deserialize(NULL, instream);
    self->total_hits = InStream_Read_C32(instream);
    return self;
}

VArray*
TopDocs_get_match_docs(TopDocs *self) { return self->match_docs; }
u32_t
TopDocs_get_total_hits(TopDocs *self) { return self->total_hits; }

void
TopDocs_set_match_docs(TopDocs *self, VArray *match_docs)
{
    DECREF(self->match_docs);
    self->match_docs = (VArray*)INCREF(match_docs);
}
void
TopDocs_set_total_hits(TopDocs *self, u32_t total_hits) 
    { self->total_hits = total_hits; }

/* Copyright 2006-2009 Marvin Humphrey
 *
 * This program is free software; you can redistribute it and/or modify
 * under the same terms as Perl itself.
 */

