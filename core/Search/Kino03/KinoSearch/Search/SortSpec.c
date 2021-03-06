#include "Search/Kino03/KinoSearch/Util/ToolSet.h"

#include "Search/Kino03/KinoSearch/Search/SortSpec.h"
#include "Search/Kino03/KinoSearch/FieldType.h"
#include "Search/Kino03/KinoSearch/Index/IndexReader.h"
#include "Search/Kino03/KinoSearch/Index/SegReader.h"
#include "Search/Kino03/KinoSearch/Schema.h"
#include "Search/Kino03/KinoSearch/Search/SortRule.h"
#include "Search/Kino03/KinoSearch/Store/InStream.h"
#include "Search/Kino03/KinoSearch/Store/OutStream.h"
#include "Search/Kino03/KinoSearch/Util/I32Array.h"
#include "Search/Kino03/KinoSearch/Util/MSort.h"

SortSpec*
SortSpec_new(VArray *rules)
{
    SortSpec *self = (SortSpec*)VTable_Make_Obj(&SORTSPEC);
    return SortSpec_init(self, rules);
}

SortSpec*
SortSpec_init(SortSpec *self, VArray *rules)
{
    i32_t i, max;
    self->rules = VA_Shallow_Copy(rules);
    for (i = 0, max = VA_Get_Size(rules); i < max; i++) {
        SortRule *rule = (SortRule*)VA_Fetch(rules, i);
        ASSERT_IS_A(rule, SORTRULE);
    }
    return self;
}

void
SortSpec_destroy(SortSpec *self)
{
    DECREF(self->rules);
    FREE_OBJ(self);
}

SortSpec*
SortSpec_deserialize(SortSpec *self, InStream *instream)
{
    u32_t num_rules = InStream_Read_C32(instream);
    VArray *rules = VA_new(num_rules);
    u32_t i;

    /* Create base object. */
    self = self ? self : (SortSpec*)VTable_Make_Obj(&SORTSPEC);

    /* Add rules. */
    for (i = 0; i < num_rules; i++) {
        VA_Push(rules, (Obj*)SortRule_deserialize(NULL, instream));
    }
    SortSpec_init(self, rules);
    DECREF(rules);

    return self;
}

VArray*
SortSpec_get_rules(SortSpec *self) { return self->rules; }

void
SortSpec_serialize(SortSpec *self, OutStream *target)
{
    u32_t num_rules = VA_Get_Size(self->rules);
    u32_t i;
    OutStream_Write_C32(target, num_rules);
    for (i = 0; i < num_rules; i++) {
        SortRule *rule = (SortRule*)VA_Fetch(self->rules, i);
        SortRule_Serialize(rule, target);
    }
}

/* Copyright 2007-2009 Marvin Humphrey
 *
 * This program is free software; you can redistribute it and/or modify
 * under the same terms as Perl itself.
 */

