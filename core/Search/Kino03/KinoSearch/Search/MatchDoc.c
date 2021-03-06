#include "Search/Kino03/KinoSearch/Util/ToolSet.h"

#include "Search/Kino03/KinoSearch/Search/MatchDoc.h"
#include "Search/Kino03/KinoSearch/Store/InStream.h"
#include "Search/Kino03/KinoSearch/Store/OutStream.h"

MatchDoc*
MatchDoc_new(i32_t doc_id, float score, VArray *values)
{
    MatchDoc *self = (MatchDoc*)VTable_Make_Obj(&MATCHDOC);
    return MatchDoc_init(self, doc_id, score, values);
}

MatchDoc*
MatchDoc_init(MatchDoc *self, i32_t doc_id, float score, VArray *values)
{
    self->doc_id      = doc_id;
    self->score       = score;
    self->values      = values ? (VArray*)INCREF(values) : NULL;
    return self;
}

void
MatchDoc_destroy(MatchDoc *self)
{
    DECREF(self->values);
    FREE_OBJ(self);
}

void
MatchDoc_serialize(MatchDoc *self, OutStream *outstream)
{
    OutStream_Write_C32(outstream, self->doc_id);
    OutStream_Write_Float(outstream, self->score);
    OutStream_Write_U8(outstream, self->values ? 1 : 0);
    if (self->values) { VA_Serialize(self->values, outstream); }
}

MatchDoc*
MatchDoc_deserialize(MatchDoc *self, InStream *instream)
{
    self = self ? self : (MatchDoc*)VTable_Make_Obj(&MATCHDOC);
    self->doc_id = InStream_Read_C32(instream);
    self->score  = InStream_Read_Float(instream);
    if (InStream_Read_U8(instream)) {
        self->values = VA_deserialize(NULL, instream);
    }
    return self;
}

i32_t
MatchDoc_get_doc_id(MatchDoc *self) { return self->doc_id; }
float
MatchDoc_get_score(MatchDoc *self)  { return self->score; }
VArray*
MatchDoc_get_values(MatchDoc *self) { return self->values; }
void
MatchDoc_set_doc_id(MatchDoc *self, i32_t doc_id) 
    { self->doc_id = doc_id; }
void
MatchDoc_set_score(MatchDoc *self, float score) 
    { self->score = score; }
void
MatchDoc_set_values(MatchDoc *self, VArray *values)
{
    DECREF(self->values);
    self->values = values ? (VArray*)INCREF(values) : NULL;
}

/* Copyright 2006-2009 Marvin Humphrey
 *
 * This program is free software; you can redistribute it and/or modify
 * under the same terms as Perl itself.
 */

