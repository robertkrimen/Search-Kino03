#include "Search/Kino03/KinoSearch/Util/ToolSet.h"

#include "Search/Kino03/KinoSearch/Analysis/CaseFolder.h"
#include "Search/Kino03/KinoSearch/Analysis/Token.h"
#include "Search/Kino03/KinoSearch/Analysis/Inversion.h"
#include "Search/Kino03/KinoSearch/Util/ByteBuf.h"

CaseFolder*
CaseFolder_new()
{
    CaseFolder *self = (CaseFolder*)VTable_Make_Obj(&CASEFOLDER);
    return CaseFolder_init(self);
}

CaseFolder*
CaseFolder_init(CaseFolder *self)
{
    Analyzer_init((Analyzer*)self);
    self->work_buf = BB_new(0);
    return self;
}

void
CaseFolder_destroy(CaseFolder *self)
{
    DECREF(self->work_buf);
    FREE_OBJ(self);
}

bool_t
CaseFolder_equals(CaseFolder *self, Obj *other)
{
    CaseFolder *const evil_twin = (CaseFolder*)other;
    if (evil_twin == self) return true;
    UNUSED_VAR(self);
    if (!OBJ_IS_A(evil_twin, CASEFOLDER)) return false;
    return true;
}

Hash*
CaseFolder_dump(CaseFolder *self)
{
    CaseFolder_dump_t super_dump 
        = (CaseFolder_dump_t)SUPER_METHOD(&CASEFOLDER, CaseFolder, Dump);
    return super_dump(self);
}

CaseFolder*
CaseFolder_load(CaseFolder *self, Obj *dump)
{
    CaseFolder_load_t super_load 
        = (CaseFolder_load_t)SUPER_METHOD(&CASEFOLDER, CaseFolder, Load);
    CaseFolder *loaded = super_load(self, dump);
    return CaseFolder_init(loaded);
}

/* Copyright 2005-2009 Marvin Humphrey
 *
 * This program is free software; you can redistribute it and/or modify
 * under the same terms as Perl itself.
 */

