#include "Search/Kino03/KinoSearch/Util/ToolSet.h"

#include <string.h>

#include "Search/Kino03/KinoSearch/Util/Freezer.h"
#include "Search/Kino03/KinoSearch/Store/InStream.h"
#include "Search/Kino03/KinoSearch/Store/OutStream.h"

void
Freezer_freeze(Obj *obj, OutStream *outstream)
{
    CB_Serialize(Obj_Get_Class_Name(obj), outstream);
    Obj_Serialize(obj, outstream);
}

Obj*
Freezer_thaw(InStream *instream)
{
    CharBuf *class_name = CB_deserialize(NULL, instream);
    VTable *vtable = (VTable*)VTable_singleton(class_name, NULL);
    Obj *blank = VTable_Make_Obj(vtable);
    DECREF(class_name);
    return Obj_Deserialize(blank, instream);
}

/* Copyright 2007-2009 Marvin Humphrey
 *
 * This program is free software; you can redistribute it and/or modify
 * under the same terms as Perl itself.
 */

