#include "Search/Kino03/KinoSearch/Util/ToolSet.h"

#include "Search/Kino03/KinoSearch/Index/DataWriter.h"
#include "Search/Kino03/KinoSearch/Schema.h"
#include "Search/Kino03/KinoSearch/Store/Folder.h"
#include "Search/Kino03/KinoSearch/Index/PolyReader.h"
#include "Search/Kino03/KinoSearch/Index/Segment.h"
#include "Search/Kino03/KinoSearch/Index/SegReader.h"
#include "Search/Kino03/KinoSearch/Index/Snapshot.h"
#include "Search/Kino03/KinoSearch/Store/Folder.h"
#include "Search/Kino03/KinoSearch/Util/I32Array.h"

DataWriter*
DataWriter_init(DataWriter *self, Snapshot *snapshot, Segment *segment, 
                PolyReader *polyreader)
{
    Schema   *schema   = PolyReader_Get_Schema(polyreader);
    Folder   *folder   = PolyReader_Get_Folder(polyreader);

    /* Assign. */
    self->snapshot   = (Snapshot*)INCREF(snapshot);
    self->segment    = (Segment*)INCREF(segment);
    self->polyreader = (PolyReader*)INCREF(polyreader);
    self->schema     = (Schema*)INCREF(schema);
    self->folder     = (Folder*)INCREF(folder);

    ABSTRACT_CLASS_CHECK(self, DATAWRITER);
    return self;
}

void
DataWriter_destroy(DataWriter *self) 
{
    DECREF(self->snapshot);
    DECREF(self->segment);
    DECREF(self->polyreader);
    DECREF(self->schema);
    DECREF(self->folder);
    FREE_OBJ(self);
}

Snapshot*
DataWriter_get_snapshot(DataWriter *self) { return self->snapshot; }
Segment*
DataWriter_get_segment(DataWriter *self)  { return self->segment; }
PolyReader*
DataWriter_get_polyreader(DataWriter *self) { return self->polyreader; }
Schema*
DataWriter_get_schema(DataWriter *self) { return self->schema; }
Folder*
DataWriter_get_folder(DataWriter *self) { return self->folder; }

void
DataWriter_merge_segment(DataWriter *self, SegReader *reader, 
                         I32Array *doc_map)
{
    DataWriter_Add_Segment(self, reader, doc_map);
    DataWriter_Delete_Segment(self, reader);
}

Hash*
DataWriter_metadata(DataWriter *self)
{
    Hash *metadata = Hash_new(0);
    Hash_Store_Str(metadata, "format", 6, 
        (Obj*)CB_newf("%i32", DataWriter_Format(self)));
    return metadata;
}

/* Copyright 2007-2009 Marvin Humphrey
 *
 * This program is free software; you can redistribute it and/or modify
 * under the same terms as Perl itself.
 */

