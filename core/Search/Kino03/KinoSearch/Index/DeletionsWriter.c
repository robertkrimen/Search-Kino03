#include "Search/Kino03/KinoSearch/Util/ToolSet.h"

#include <math.h>

#include "Search/Kino03/KinoSearch/Index/DeletionsWriter.h"
#include "Search/Kino03/KinoSearch/Index/DeletionsReader.h"
#include "Search/Kino03/KinoSearch/Index/IndexReader.h"
#include "Search/Kino03/KinoSearch/Index/PolyReader.h"
#include "Search/Kino03/KinoSearch/Index/PostingList.h"
#include "Search/Kino03/KinoSearch/Index/PostingsReader.h"
#include "Search/Kino03/KinoSearch/Index/Segment.h"
#include "Search/Kino03/KinoSearch/Index/SegReader.h"
#include "Search/Kino03/KinoSearch/Index/Snapshot.h"
#include "Search/Kino03/KinoSearch/Search/BitVecMatcher.h"
#include "Search/Kino03/KinoSearch/Search/Compiler.h"
#include "Search/Kino03/KinoSearch/Search/Matcher.h"
#include "Search/Kino03/KinoSearch/Search/Query.h"
#include "Search/Kino03/KinoSearch/Searcher.h"
#include "Search/Kino03/KinoSearch/Store/Folder.h"
#include "Search/Kino03/KinoSearch/Store/OutStream.h"
#include "Search/Kino03/KinoSearch/Util/BitVector.h"
#include "Search/Kino03/KinoSearch/Util/I32Array.h"

DeletionsWriter*
DelWriter_init(DeletionsWriter *self, Snapshot *snapshot, Segment *segment,
               PolyReader *polyreader)
{
    DataWriter_init((DataWriter*)self, snapshot, segment, polyreader);
    ABSTRACT_CLASS_CHECK(self, DELETIONSWRITER);
    return self;
}

I32Array*
DelWriter_generate_doc_map(DeletionsWriter *self, Matcher *deletions,
                           i32_t doc_max, i32_t offset) 
{
    i32_t *doc_map = CALLOCATE(doc_max + 1, i32_t);
    i32_t  new_doc_id;
    i32_t  i;
    i32_t  next_deletion = deletions ? Matcher_Next(deletions) : I32_MAX;
    UNUSED_VAR(self);

    /* 0 for a deleted doc, a new number otherwise */
    for (i = 1, new_doc_id = 1; i <= doc_max; i++) {
        if (i == next_deletion) {
            next_deletion = Matcher_Next(deletions);
        }
        else {
            doc_map[i] = offset + new_doc_id++;
        }
    }
    
    return I32Arr_new_steal(doc_map, doc_max + 1);
}

i32_t DefDelWriter_current_file_format = 1;

DefaultDeletionsWriter*
DefDelWriter_new(Snapshot *snapshot, Segment *segment, PolyReader *polyreader)
{
    DefaultDeletionsWriter *self
        = (DefaultDeletionsWriter*)VTable_Make_Obj(&DEFAULTDELETIONSWRITER);
    return DefDelWriter_init(self, snapshot, segment, polyreader); 
}

DefaultDeletionsWriter*
DefDelWriter_init(DefaultDeletionsWriter *self, Snapshot *snapshot, 
                  Segment *segment, PolyReader *polyreader)
{
    u32_t i;
    u32_t num_seg_readers;

    DataWriter_init((DataWriter*)self, snapshot, segment, polyreader);
    self->seg_readers       = PolyReader_Seg_Readers(polyreader);
    num_seg_readers         = VA_Get_Size(self->seg_readers);
    self->seg_starts        = PolyReader_Offsets(polyreader);
    self->bit_vecs          = VA_new(num_seg_readers);
    self->updated           = CALLOCATE(num_seg_readers, bool_t);
    self->searcher          = Searcher_new((Obj*)polyreader);

    /* Materialize a BitVector of deletions for each segment. */
    for (i = 0; i < num_seg_readers; i++) {
        SegReader *seg_reader = (SegReader*)VA_Fetch(self->seg_readers, i);
        BitVector *bit_vec    = BitVec_new(SegReader_Doc_Max(seg_reader));
        DeletionsReader *del_reader = (DeletionsReader*)SegReader_Fetch(
            seg_reader, DELETIONSREADER.name);
        Matcher *seg_dels = del_reader 
                          ? DelReader_Iterator(del_reader) : NULL;

        if (seg_dels) {
            i32_t del;
            while (0 != (del = Matcher_Next(seg_dels))) {
                BitVec_Set(bit_vec, del);
            }
            DECREF(seg_dels);
        }
        VA_Store(self->bit_vecs, i, (Obj*)bit_vec);
    }

    return self;
}

void
DefDelWriter_destroy(DefaultDeletionsWriter *self)
{
    DECREF(self->seg_readers);
    DECREF(self->seg_starts);
    DECREF(self->bit_vecs);
    DECREF(self->searcher);
    free(self->updated);
    SUPER_DESTROY(self, DEFAULTDELETIONSWRITER);
}

static CharBuf*
S_del_filename(DefaultDeletionsWriter *self, SegReader *target_reader)
{
    Segment *target_seg = SegReader_Get_Segment(target_reader);
    return CB_newf("%o/deletions-%o.bv", Seg_Get_Name(self->segment),
        Seg_Get_Name(target_seg));
}

void
DefDelWriter_finish(DefaultDeletionsWriter *self)
{
    Folder *const folder = self->folder;
    u32_t i, max;

    for (i = 0, max = VA_Get_Size(self->seg_readers); i < max; i++) {
        SegReader *seg_reader = (SegReader*)VA_Fetch(self->seg_readers, i);
        if (self->updated[i]) {
            BitVector *deldocs   = (BitVector*)VA_Fetch(self->bit_vecs, i);
            i32_t      doc_max   = SegReader_Doc_Max(seg_reader);
            u32_t      byte_size = (u32_t)ceil(doc_max/ 8.0);
            u32_t      new_max   = byte_size * 8 - 1;
            CharBuf   *filename  = S_del_filename(self, seg_reader);
            Snapshot  *snapshot  = DefDelWriter_Get_Snapshot(self);
            OutStream *outstream = Folder_Open_Out(folder, filename);

            if (!outstream) { THROW("Can't open %o", filename); }
            Snapshot_Add_Entry(snapshot, filename);

            /* Ensure that we have 1 bit for each doc in segment. */
            BitVec_Grow(deldocs, new_max);

            /* Write deletions data and clean up. */
            OutStream_Write_Bytes(outstream, (char*)deldocs->bits, byte_size);
            OutStream_Close(outstream);
            DECREF(outstream);
            DECREF(filename);
        }
    }

    Seg_Store_Metadata_Str(self->segment, "deletions", 9,
        (Obj*)DefDelWriter_Metadata(self));
}

Hash*
DefDelWriter_metadata(DefaultDeletionsWriter *self)
{
    Hash    *const metadata = DataWriter_metadata((DataWriter*)self);
    Hash    *const files    = Hash_new(0);
    u32_t i, max;

    for (i = 0, max = VA_Get_Size(self->seg_readers); i < max; i++) {
        SegReader *seg_reader = (SegReader*)VA_Fetch(self->seg_readers, i);
        if (self->updated[i]) {
            BitVector *deldocs   = (BitVector*)VA_Fetch(self->bit_vecs, i);
            Segment   *segment   = SegReader_Get_Segment(seg_reader);
            Hash      *mini_meta = Hash_new(2);
            Hash_Store_Str(mini_meta, "count", 5, 
                (Obj*)CB_newf("%u32", (u32_t)BitVec_Count(deldocs)) );
            Hash_Store_Str(mini_meta, "filename", 8, 
                (Obj*)S_del_filename(self, seg_reader));
            Hash_Store(files, Seg_Get_Name(segment), (Obj*)mini_meta);
        }
    }
    Hash_Store_Str(metadata, "files", 5, (Obj*)files);

    return metadata;
}

i32_t
DefDelWriter_format(DefaultDeletionsWriter *self)
{
    UNUSED_VAR(self);
    return DefDelWriter_current_file_format;
}

Matcher*
DefDelWriter_seg_deletions(DefaultDeletionsWriter *self, 
                           SegReader *seg_reader)
{
    u32_t i;
    u32_t num_seg_readers = VA_Get_Size(self->seg_readers);
    Matcher *deletions    = NULL;

    for (i = 0; i < num_seg_readers; i++) {
        SegReader *candidate = (SegReader*)VA_Fetch(self->seg_readers, i);
        if (candidate == seg_reader) {
            DeletionsReader *del_reader = (DeletionsReader*)SegReader_Obtain(
                candidate, DELETIONSREADER.name);
            if (self->updated[i] || DelReader_Del_Count(del_reader)) {
                BitVector *deldocs = (BitVector*)VA_Fetch(self->bit_vecs, i);
                deletions = (Matcher*)BitVecMatcher_new(deldocs);
            }
            break;
        }
    }
    if (i == num_seg_readers) { /* Sanity check. */
        THROW("Couldn't find SegReader %o", seg_reader);
    }

    return deletions;
}

void
DefDelWriter_delete_by_term(DefaultDeletionsWriter *self,
                            const CharBuf *field, Obj *term)
{
    u32_t i, max;
    for (i = 0, max = VA_Get_Size(self->seg_readers); i < max; i++) {
        SegReader *seg_reader = (SegReader*)VA_Fetch(self->seg_readers, i);
        PostingsReader *post_reader = (PostingsReader*)SegReader_Fetch(
            seg_reader, POSTINGSREADER.name);
        BitVector *bit_vec = (BitVector*)VA_Fetch(self->bit_vecs, i);
        PostingList *plist = post_reader 
            ? PostReader_Posting_List(post_reader, field, term) : NULL;
        i32_t doc_id;
        i32_t num_zapped = 0;

        /* Iterate through postings, marking each doc as deleted. */
        if (plist) {
            while (0 != (doc_id = PList_Next(plist))) {
                num_zapped += !BitVec_Get(bit_vec, doc_id);
                BitVec_Set(bit_vec, doc_id);
            }
            if (num_zapped) { self->updated[i] = true; }
            DECREF(plist);
        }
    }
}

void
DefDelWriter_delete_by_query(DefaultDeletionsWriter *self, Query *query)
{
    Compiler *compiler = Query_Make_Compiler(query, 
        (Searchable*)self->searcher, Query_Get_Boost(query));
    u32_t i, max;

    for (i = 0, max = VA_Get_Size(self->seg_readers); i < max; i++) {
        SegReader *seg_reader = (SegReader*)VA_Fetch(self->seg_readers, i);
        BitVector *bit_vec = (BitVector*)VA_Fetch(self->bit_vecs, i);
        Matcher *matcher = Compiler_Make_Matcher(compiler, seg_reader, false);

        if (matcher) { 
            i32_t doc_id;
            i32_t num_zapped = 0;

            /* Iterate through matches, marking each doc as deleted. */
            while (0 != (doc_id = Matcher_Next(matcher))) {
                num_zapped += !BitVec_Get(bit_vec, doc_id);
                BitVec_Set(bit_vec, doc_id);
            }
            if (num_zapped) { self->updated[i] = true; }

            DECREF(matcher);
        }
    }

    DECREF(compiler);
}

bool_t
DefDelWriter_updated(DefaultDeletionsWriter *self)
{
    u32_t i, max;
    for (i = 0, max = VA_Get_Size(self->seg_readers); i < max; i++) {
        if (self->updated[i]) { return true; }
    }
    return false;
}

void
DefDelWriter_add_segment(DefaultDeletionsWriter *self, SegReader *reader, 
                         I32Array *doc_map)
{
    /* This method is a no-op, because the only reason it would be called is
     * if we are adding an entire index.  If that's the case, all deletes are
     * already being applied.
     */
    UNUSED_VAR(self);
    UNUSED_VAR(reader);
    UNUSED_VAR(doc_map);
}

static void
S_zap_segment(DefaultDeletionsWriter *self, SegReader *reader,
              bool_t merge_deletions)
{
    Snapshot *snapshot = DefDelWriter_Get_Snapshot(self);
    Segment *segment = SegReader_Get_Segment(reader);
    Hash *del_meta = (Hash*)Seg_Fetch_Metadata_Str(segment, "deletions", 9);

    if (del_meta) {
        VArray *seg_readers = self->seg_readers;
        Hash   *files = (Hash*)Hash_Fetch_Str(del_meta, "files", 5);
        if (files) {
            CharBuf *seg_name;
            Hash *mini_meta;
            Hash_Iter_Init(files);
            while(Hash_Iter_Next(files, &seg_name, (Obj**)&mini_meta)) {
                u32_t i, max;
                CharBuf *filename = (CharBuf*)ASSERT_IS_A(
                    Hash_Fetch_Str(mini_meta, "filename", 8), CHARBUF);

                /* Remove file from snapshot. */
                Snapshot_Delete_Entry(snapshot, filename);

                /* This is where Merge_Segment and Delete_Segment diverge. */
                if (!merge_deletions) { continue; }

                /* Find the segment the deletions from the SegReader
                 * we're adding correspond to.  If it's gone, we don't
                 * need to worry about losing deletions files that point
                 * at it. */
                for (i = 0, max = VA_Get_Size(seg_readers); i < max; i++) {
                    SegReader *candidate
                        = (SegReader*)VA_Fetch(seg_readers, i);
                    CharBuf *candidate_name 
                        = Seg_Get_Name(SegReader_Get_Segment(candidate));

                    if (CB_Equals(seg_name, (Obj*)candidate_name)) { 
                        /* If the count hasn't changed, we're about to
                         * merge away the most recent deletions file
                         * pointing at this target segment -- so force a
                         * new file to be written out. */
                        i32_t count = (i32_t)Obj_To_I64(
                            Hash_Fetch_Str(mini_meta, "count", 5));
                        DeletionsReader *del_reader = (DeletionsReader*)
                            SegReader_Obtain(candidate, DELETIONSREADER.name);
                        if (count == DelReader_Del_Count(del_reader)) {
                            self->updated[i] = true;
                        }
                        break;
                    }
                }
            }
        }
    }
}

void
DefDelWriter_merge_segment(DefaultDeletionsWriter *self, SegReader *reader, 
                           I32Array *doc_map)
{
    UNUSED_VAR(doc_map);
    S_zap_segment(self, reader, true);
}

void
DefDelWriter_delete_segment(DefaultDeletionsWriter *self, SegReader *reader)
{
    S_zap_segment(self, reader, false);
}

/* Copyright 2006-2009 Marvin Humphrey
 *
 * This program is free software; you can redistribute it and/or modify
 * under the same terms as Perl itself.
 */

