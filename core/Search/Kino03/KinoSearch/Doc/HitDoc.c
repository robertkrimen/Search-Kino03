#include "Search/Kino03/KinoSearch/Util/ToolSet.h"

#include "Search/Kino03/KinoSearch/Doc/HitDoc.h"
#include "Search/Kino03/KinoSearch/Store/InStream.h"
#include "Search/Kino03/KinoSearch/Store/OutStream.h"

HitDoc*
HitDoc_new(void *fields, i32_t doc_id, float score)
{
    HitDoc *self = (HitDoc*)VTable_Make_Obj(&HITDOC);
    return HitDoc_init(self, fields, doc_id, score);
}

HitDoc*
HitDoc_init(HitDoc *self, void *fields, i32_t doc_id, float score)
{
    Doc_init((Doc*)self, fields, doc_id);
    self->score = score;
    return self;
}

void  HitDoc_set_score(HitDoc *self, float score) { self->score = score; }
float HitDoc_get_score(HitDoc *self)              { return self->score;  }

void
HitDoc_serialize(HitDoc *self, OutStream *outstream)
{
    Doc_serialize((Doc*)self, outstream);
    OutStream_Write_Float(outstream, self->score);
}

HitDoc*
HitDoc_deserialize(HitDoc *self, InStream *instream)
{
    self = self ? self : (HitDoc*)VTable_Make_Obj(&HITDOC);
    Doc_deserialize((Doc*)self, instream);
    self->score = InStream_Read_Float(instream);
    return self;
}

Hash*
HitDoc_dump(HitDoc *self)
{
    HitDoc_dump_t super_dump 
        = (HitDoc_dump_t)SUPER_METHOD(&HITDOC, HitDoc, Dump);
    Hash *dump = super_dump(self);
    Hash_Store_Str(dump, "score", 5, (Obj*)CB_newf("%f64", self->score));
    return dump;
}

HitDoc*
HitDoc_load(HitDoc *self, Obj *dump)
{
    Hash *source = (Hash*)ASSERT_IS_A(dump, HASH);
    HitDoc_load_t super_load 
        = (HitDoc_load_t)SUPER_METHOD(&HITDOC, HitDoc, Load);
    HitDoc *loaded = super_load(self, dump);
    Obj *score = ASSERT_IS_A(Hash_Fetch_Str(source, "score", 5), OBJ);
    loaded->score = (float)Obj_To_F64(score);
    return loaded;
}

bool_t
HitDoc_equals(HitDoc *self, Obj *other)
{
    HitDoc *evil_twin = (HitDoc*)other;
    if (evil_twin == self)                { return true;  }
    if (!OBJ_IS_A(evil_twin, HITDOC))     { return false; }
    if (!Doc_equals((Doc*)self, other))   { return false; }
    if (!self->score == evil_twin->score) { return false; }
    return true;
}

/* Copyright 2006-2009 Marvin Humphrey
 *
 * This program is free software; you can redistribute it and/or modify
 * under the same terms as Perl itself.
 */

