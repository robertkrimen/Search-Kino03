#include "Search/Kino03/KinoSearch/Util/ToolSet.h"

#include "Search/Kino03/KinoSearch/Index/DeletionsReader.h"
#include "Search/Kino03/KinoSearch/Schema.h"
#include "Search/Kino03/KinoSearch/Index/BitVecDelDocs.h"
#include "Search/Kino03/KinoSearch/Index/DeletionsWriter.h"
#include "Search/Kino03/KinoSearch/Index/Segment.h"
#include "Search/Kino03/KinoSearch/Index/Snapshot.h"
#include "Search/Kino03/KinoSearch/Search/BitVecMatcher.h"
#include "Search/Kino03/KinoSearch/Search/SeriesMatcher.h"
#include "Search/Kino03/KinoSearch/Store/Folder.h"
#include "Search/Kino03/KinoSearch/Store/InStream.h"
#include "Search/Kino03/KinoSearch/Util/BitVector.h"
#include "Search/Kino03/KinoSearch/Util/I32Array.h"
#include "Search/Kino03/KinoSearch/Util/IndexFileNames.h"

DeletionsReader*
DelReader_init(DeletionsReader *self, Schema *schema, Folder *folder, 
               Snapshot *snapshot, VArray *segments, i32_t seg_tick)
{
    DataReader_init((DataReader*)self, schema, folder, snapshot, segments, 
        seg_tick);
    ABSTRACT_CLASS_CHECK(self, DELETIONSREADER);
    return self;
}

DeletionsReader*
DelReader_aggregator(DeletionsReader *self, VArray *readers, 
                     I32Array *offsets)
{
    UNUSED_VAR(self);
    return (DeletionsReader*)PolyDelReader_new(readers, offsets);
}

PolyDeletionsReader*
PolyDelReader_new(VArray *readers, I32Array *offsets)
{
    PolyDeletionsReader *self 
        = (PolyDeletionsReader*)VTable_Make_Obj(&POLYDELETIONSREADER);
    return PolyDelReader_init(self, readers, offsets);
}

PolyDeletionsReader*
PolyDelReader_init(PolyDeletionsReader *self, VArray *readers, I32Array *offsets)
{
    u32_t i, max;
    DelReader_init((DeletionsReader*)self, NULL, NULL, NULL, NULL, -1);
    self->del_count = 0;
    for (i = 0, max = VA_Get_Size(readers); i < max; i++) {
        DeletionsReader *reader = (DeletionsReader*) ASSERT_IS_A(
            VA_Fetch(readers, i), DELETIONSREADER);
        self->del_count += DelReader_Del_Count(reader);
    }
    self->readers = (VArray*)INCREF(readers);
    self->offsets = (I32Array*)INCREF(offsets);
    return self;
}

void
PolyDelReader_close(PolyDeletionsReader *self)
{
    if (self->readers) {
        u32_t i, max;
        for (i = 0, max = VA_Get_Size(self->readers); i < max; i++) {
            DeletionsReader *reader 
                = (DeletionsReader*)VA_Fetch(self->readers, i);
            if (reader) { DelReader_Close(reader); }
        }
        VA_Clear(self->readers);
    }
}

void
PolyDelReader_destroy(PolyDeletionsReader *self)
{
    DECREF(self->readers);
    DECREF(self->offsets);
    SUPER_DESTROY(self, POLYDELETIONSREADER);
}

i32_t
PolyDelReader_del_count(PolyDeletionsReader *self)
{
    return self->del_count;
}

Matcher*
PolyDelReader_iterator(PolyDeletionsReader *self)
{
    SeriesMatcher *deletions = NULL;
    if (self->del_count) {
        u32_t num_readers = VA_Get_Size(self->readers);
        VArray *matchers = VA_new(num_readers);
        u32_t i;
        for (i = 0; i < num_readers; i++) {
            DeletionsReader *reader 
                = (DeletionsReader*)VA_Fetch(self->readers, i);
            Matcher *matcher = DelReader_Iterator(reader);
            if (matcher) { VA_Store(matchers, i, (Obj*)matcher); }
        }
        deletions = SeriesMatcher_new(matchers, self->offsets);
        DECREF(matchers);
    }
    return (Matcher*)deletions;
}

DefaultDeletionsReader*
DefDelReader_new(Schema *schema, Folder *folder, Snapshot *snapshot,
                    VArray *segments, i32_t seg_tick)
{
    DefaultDeletionsReader *self 
        = (DefaultDeletionsReader*)VTable_Make_Obj(&DEFAULTDELETIONSREADER);
    return DefDelReader_init(self, schema, folder, snapshot, segments,
        seg_tick);
}

DefaultDeletionsReader*
DefDelReader_init(DefaultDeletionsReader *self, Schema *schema, 
                  Folder *folder, Snapshot *snapshot, VArray *segments,
                  i32_t seg_tick)
{
    DelReader_init((DeletionsReader*)self, schema, folder, snapshot, segments,
        seg_tick);
    DefDelReader_Read_Deletions(self);
    if (!self->deldocs) { 
        self->del_count = 0;
        self->deldocs   = BitVec_new(0); 
    }
    return self;
}

void
DefDelReader_close(DefaultDeletionsReader *self)
{
    DECREF(self->deldocs);
    self->deldocs = NULL;
}

void
DefDelReader_destroy(DefaultDeletionsReader *self)
{
    DECREF(self->deldocs);
    SUPER_DESTROY(self, DEFAULTDELETIONSREADER);
}

BitVector*
DefDelReader_read_deletions(DefaultDeletionsReader *self)
{
    VArray  *segments    = DefDelReader_Get_Segments(self);
    Segment *segment     = DefDelReader_Get_Segment(self);
    CharBuf *my_seg_name = Seg_Get_Name(segment);
    CharBuf *del_file    = NULL;
    i32_t    del_count   = 0;
    i32_t i;
    
    /* Start with deletions files in the most recently added segments and work
     * backwards.  The first one we find which addresses our segment is the
     * one we need. */
    for (i = VA_Get_Size(segments) - 1; i >= 0; i--) {
        Segment *other_seg = (Segment*)VA_Fetch(segments, i);
        Hash *metadata 
            = (Hash*)Seg_Fetch_Metadata_Str(other_seg, "deletions", 9);
        if (metadata) {
            Hash *files = (Hash*)ASSERT_IS_A(
                Hash_Fetch_Str(metadata, "files", 5), HASH);
            Hash *seg_files_data = (Hash*)Hash_Fetch(files, my_seg_name);
            if (seg_files_data) {
                Obj *count = (Obj*)ASSERT_IS_A(
                    Hash_Fetch_Str(seg_files_data, "count", 5), OBJ);
                del_count = Obj_To_I64(count);
                del_file  = (CharBuf*)ASSERT_IS_A(
                    Hash_Fetch_Str(seg_files_data, "filename", 8), CHARBUF);
                break;
            }
        }
    }

    DECREF(self->deldocs);
    if (del_file) {
        self->deldocs = (BitVector*)BitVecDelDocs_new(self->folder, del_file);
        self->del_count = del_count;
    }
    else {
        self->deldocs = NULL;
        self->del_count = 0;
    }

    return self->deldocs;
}

Matcher*
DefDelReader_iterator(DefaultDeletionsReader *self)
{
    return (Matcher*)BitVecMatcher_new(self->deldocs);
}

i32_t
DefDelReader_del_count(DefaultDeletionsReader *self) 
{ 
    return self->del_count; 
}

/* Copyright 2006-2009 Marvin Humphrey
 *
 * This program is free software; you can redistribute it and/or modify
 * under the same terms as Perl itself.
 */

