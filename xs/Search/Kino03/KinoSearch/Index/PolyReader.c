#include "Search/Kino03/KinoSearch/Util/ToolSet.h"

#include "Search/Kino03/KinoSearch/Index/PolyReader.h"
#include "Search/Kino03/KinoSearch/Index/Snapshot.h"
#include "Search/Kino03/KinoSearch/Store/Folder.h"
#include "Search/Kino03/KinoSearch/Store/LockFactory.h"
#include "Search/Kino03/KinoSearch/Util/Host.h"

Obj*
PolyReader_try_open_segreaders(PolyReader *self, VArray *segments)
{
    return Host_callback_obj(self, "try_open_segreaders", 1, 
        ARG_OBJ("segments", segments));
}

CharBuf*
PolyReader_try_read_snapshot(Snapshot *snapshot, Folder *folder, 
                             const CharBuf *filename) 
{
    return (CharBuf*)Host_callback_obj(&POLYREADER, "try_read_snapshot", 3,
        ARG_OBJ("snapshot", snapshot), ARG_OBJ("folder", folder), 
        ARG_STR("filename", filename));
}

/* Copyright 2006-2009 Marvin Humphrey
 *
 * This program is free software; you can redistribute it and/or modify
 * under the same terms as Perl itself.
 */

