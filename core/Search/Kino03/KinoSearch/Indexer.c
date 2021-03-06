#include "Search/Kino03/KinoSearch/Util/ToolSet.h"

#include "Search/Kino03/KinoSearch/Indexer.h"
#include "Search/Kino03/KinoSearch/Analysis/Analyzer.h"
#include "Search/Kino03/KinoSearch/Architecture.h"
#include "Search/Kino03/KinoSearch/Doc.h"
#include "Search/Kino03/KinoSearch/FieldType.h"
#include "Search/Kino03/KinoSearch/FieldType/FullTextType.h"
#include "Search/Kino03/KinoSearch/Schema.h"
#include "Search/Kino03/KinoSearch/Index/DeletionsReader.h"
#include "Search/Kino03/KinoSearch/Index/DeletionsWriter.h"
#include "Search/Kino03/KinoSearch/Index/FilePurger.h"
#include "Search/Kino03/KinoSearch/Index/IndexManager.h"
#include "Search/Kino03/KinoSearch/Index/PolyReader.h"
#include "Search/Kino03/KinoSearch/Index/Segment.h"
#include "Search/Kino03/KinoSearch/Index/SegReader.h"
#include "Search/Kino03/KinoSearch/Index/Snapshot.h"
#include "Search/Kino03/KinoSearch/Index/SegWriter.h"
#include "Search/Kino03/KinoSearch/Search/Matcher.h"
#include "Search/Kino03/KinoSearch/Search/Query.h"
#include "Search/Kino03/KinoSearch/Store/Folder.h"
#include "Search/Kino03/KinoSearch/Store/FSFolder.h"
#include "Search/Kino03/KinoSearch/Store/Lock.h"
#include "Search/Kino03/KinoSearch/Store/LockFactory.h"
#include "Search/Kino03/KinoSearch/Util/I32Array.h"
#include "Search/Kino03/KinoSearch/Util/IndexFileNames.h"
#include "Search/Kino03/KinoSearch/Util/MathUtils.h"

i32_t Indexer_CREATE   = 0x00000001;
i32_t Indexer_TRUNCATE = 0x00000002;

/* Release the write lock - if it's there. */
static void
S_release_write_lock(Indexer *self);

/* Verify a Folder or derive an FSFolder from a CharBuf path.  Call
 * Folder_Initialize() if "create" is true. */
static Folder*
S_init_folder(Obj *index, bool_t create);

Indexer*
Indexer_new(Schema *schema, Obj *index, IndexManager *manager,
            LockFactory *lock_factory, i32_t flags)
{
    Indexer *self = (Indexer*)VTable_Make_Obj(&INDEXER);
    return Indexer_init(self, schema, index, manager, lock_factory, flags);
}

Indexer*
Indexer_init(Indexer *self, Schema *schema, Obj *index, 
          IndexManager *manager, LockFactory *lock_factory, i32_t flags)
          
{
    bool_t    create   = (flags & Indexer_CREATE)   ? true : false;
    bool_t    truncate = (flags & Indexer_TRUNCATE) ? true : false;
    Folder   *folder   = S_init_folder(index, create);
    Lock     *write_lock;
    CharBuf  *latest_snapfile;
    Snapshot *latest_snapshot = Snapshot_new();

    /* Init. */
    self->stock_doc    = Doc_new(NULL, 0);
    self->truncate     = false;
    self->optimized    = false;
    self->prepared     = false;
    self->needs_commit = false;
    self->snapfile     = NULL;

    /* Assign. */
    self->schema       = (Schema*)INCREF(schema);
    self->folder       = folder;
    self->manager      = manager 
                       ? (IndexManager*)INCREF(manager) 
                       : IxManager_new(folder);
    self->lock_factory = lock_factory 
                       ? (LockFactory*)INCREF(lock_factory)
                       : LockFact_new(folder, (CharBuf*)&EMPTY);

    /* Get a write lock for this folder. */
    write_lock = LockFact_Make_Lock(self->lock_factory,
        (CharBuf*)Lock_write_lock_name, Lock_write_lock_timeout);
    Lock_Clear_Stale(write_lock);
    if (Lock_Obtain(write_lock)) {
        /* Only assign if successful, otherwise DESTROY unlocks -- bad! */
        self->write_lock = write_lock;
    }
    else {
        CharBuf *message = MAKE_MESS("index is locked by '%o'", 
            Lock_Get_Filename(write_lock));
        DECREF(write_lock);
        DECREF(self);
        Err_throw_mess(message);
    }

    /* Find the latest snapshot or create a new one. */
    latest_snapfile = IxFileNames_latest_snapshot(folder);
    if (latest_snapfile) {
        Snapshot_Read_File(latest_snapshot, folder, latest_snapfile);
    }

    /* If we're clobbering, start with an empty Snapshot and an empty 
     * PolyReader.  Otherwise, start with the most recent Snapshot and an
     * up-to-date PolyReader. */
    if (truncate) {
        self->snapshot = Snapshot_new();
        self->polyreader = PolyReader_new(schema, folder, NULL, NULL, NULL);
        self->truncate = true;
    }
    else {
        /* TODO: clone most recent snapshot rather than read it twice. */
        self->snapshot = (Snapshot*)INCREF(latest_snapshot);
        self->polyreader = latest_snapfile
            ? PolyReader_open((Obj*)folder, NULL, NULL)
            : PolyReader_new(schema, folder, NULL, NULL, NULL);

        if (latest_snapfile) {
            /* This is seriously awful hack.  For now, the Schema must be
             * shared by the Indexer and the PolyReader, because DataWriter
             * sub-components don't get access to the Indexer's schema and
             * that can cause problems when fields are added.
             *
             * TODO: Kill this hack by fixing Architecture.
             */
            Schema *old_schema = PolyReader_Get_Schema(self->polyreader);
            Schema_Eat(schema, old_schema);
            DECREF(old_schema);
            self->polyreader->schema = (Schema*)INCREF(schema);
        }
    }

    /* Zap detritus from previous sessions. */
    {
        /* Note: we have to feed FilePurger with the most recent snapshot file
         * now, but with the Indexer's snapshot later. */
        FilePurger *file_purger 
            = FilePurger_new(folder, latest_snapshot, self->lock_factory);
        FilePurger_Purge(file_purger);
        DECREF(file_purger);
    }

    /* Create new Segment, SegWriter, and FilePurger.  */
    self->file_purger 
        = FilePurger_new(folder, self->snapshot, self->lock_factory);
    self->segment 
        = IxManager_Make_New_Segment(self->manager, latest_snapshot);
    {
        VArray *fields = Schema_All_Fields(schema);
        u32_t i, max;
        for (i = 0, max = VA_Get_Size(fields); i < max; i++) {
            Seg_Add_Field(self->segment, (CharBuf*)VA_Fetch(fields, i));
        }
        DECREF(fields);
    }
    self->seg_writer = SegWriter_new(self->snapshot, self->segment, 
        self->polyreader); 
    SegWriter_Prep_Seg_Dir(self->seg_writer);

    /* Grab a local ref to the DeletionsWriter. */
    self->del_writer = (DeletionsWriter*)INCREF(
        SegWriter_Get_Del_Writer(self->seg_writer));

    DECREF(latest_snapfile);
    DECREF(latest_snapshot);

    return self;
}

void
Indexer_destroy(Indexer *self) 
{
    S_release_write_lock(self);
    DECREF(self->schema);
    DECREF(self->folder);
    DECREF(self->segment);
    DECREF(self->manager);
    DECREF(self->stock_doc);
    DECREF(self->lock_factory);
    DECREF(self->polyreader);
    DECREF(self->del_writer);
    DECREF(self->snapshot);
    DECREF(self->seg_writer);
    DECREF(self->file_purger);
    DECREF(self->write_lock);
    DECREF(self->snapfile);
    FREE_OBJ(self);
}

static Folder*
S_init_folder(Obj *index, bool_t create)
{
    Folder *folder = NULL;

    /* Validate or acquire a Folder. */
    if (OBJ_IS_A(index, FOLDER)) {
        folder = (Folder*)INCREF(index);
    }
    else if (OBJ_IS_A(index, CHARBUF)) {
        folder = (Folder*)FSFolder_new((CharBuf*)index);
    }
    else {
        THROW("Invalid type for 'index': %o", Obj_Get_Class_Name(index));
    }

    /* Validate or create the index directory. */
    if (create) { 
        Folder_Initialize(folder); 
    }
    else { 
        if (!Folder_Check(folder)) {
            THROW("Folder '%o' failed check", Folder_Get_Path(folder));
        }
    }

    return folder;
}

void
Indexer_add_doc(Indexer *self, Doc *doc, float boost)
{
    SegWriter_Add_Doc(self->seg_writer, doc, boost);
}

void
Indexer_delete_by_term(Indexer *self, CharBuf *field, Obj *term)
{
    Schema    *schema = self->schema;
    FieldType *type   = Schema_Fetch_Type(schema, field);

    /* Raise exception if the field isn't indexed. */
    if (!type || !FType_Indexed(type)) 
        THROW("%o is not an indexed field", field);

    /* Analyze term if appropriate, then zap. */
    if (OBJ_IS_A(type, FULLTEXTTYPE)) {
        ASSERT_IS_A(term, CHARBUF);
        {
            Analyzer *analyzer = Schema_Fetch_Analyzer(schema, field);
            VArray *terms = Analyzer_Split(analyzer, (CharBuf*)term);
            Obj *analyzed_term = VA_Fetch(terms, 0);
            if (analyzed_term) {
                DelWriter_Delete_By_Term(self->del_writer, field,
                    analyzed_term);
            }
            DECREF(terms);
        }
    }
    else {
        DelWriter_Delete_By_Term(self->del_writer, field, term);
    }
}

void
Indexer_delete_by_query(Indexer *self, Query *query)
{
    DelWriter_Delete_By_Query(self->del_writer, query);
}

void
Indexer_add_index(Indexer *self, Obj *index)
{
    Folder *other_folder = NULL;
    IndexReader *reader  = NULL;
    
    if (OBJ_IS_A(index, FOLDER)) {
        other_folder = (Folder*)INCREF(index);
    }
    else if (OBJ_IS_A(index, CHARBUF)) {
        other_folder = (Folder*)FSFolder_new((CharBuf*)index);
    }
    else {
        THROW("Invalid type for 'index': %o", Obj_Get_Class_Name(index));
    }

    reader = IxReader_open((Obj*)other_folder, NULL, NULL);
    if (reader == NULL) {
        THROW("Index doesn't seem to contain any data");
    }
    else {
        Schema *schema       = self->schema;
        Schema *other_schema = IxReader_Get_Schema(reader);
        VArray *other_fields = Schema_All_Fields(other_schema);
        VArray *seg_readers  = IxReader_Seg_Readers(reader);
        u32_t i, max;

        /* Validate schema compatibility and add fields. */
        Schema_Eat(schema, other_schema);

        /* Add fields to Segment. */
        for (i = 0, max = VA_Get_Size(other_fields); i < max; i++) {
            CharBuf *other_field = (CharBuf*)VA_Fetch(other_fields, i);
            Seg_Add_Field(self->segment, other_field);
        }
        DECREF(other_fields);

        /* Add all segments. */
        for (i = 0, max = VA_Get_Size(seg_readers); i < max; i++) {
            SegReader *seg_reader = (SegReader*)VA_Fetch(seg_readers, i);
            DeletionsReader *del_reader = (DeletionsReader*)SegReader_Fetch(
                seg_reader, DELETIONSREADER.name);
            Matcher *deletions = del_reader 
                               ? DelReader_Iterator(del_reader) 
                               : NULL;
            I32Array *doc_map = DelWriter_Generate_Doc_Map(self->del_writer,
                deletions, SegReader_Doc_Max(seg_reader),
                Seg_Get_Count(self->segment) 
            );
            SegWriter_Add_Segment(self->seg_writer, seg_reader, doc_map);
            DECREF(deletions);
            DECREF(doc_map);
        }
        DECREF(seg_readers);
    }

    DECREF(reader);
    DECREF(other_folder);
}

void
Indexer_optimize(Indexer *self)
{
    VArray *seg_readers = PolyReader_Get_Seg_Readers(self->polyreader);
    u32_t i, max;

    /* Consolidate segments  optimizing forced. */
    for (i = 0, max = VA_Get_Size(seg_readers); i < max; i++) {
        SegReader *seg_reader = (SegReader*)VA_Fetch(seg_readers, i);
        Matcher *deletions 
            = DelWriter_Seg_Deletions(self->del_writer, seg_reader);
        I32Array *doc_map = DelWriter_Generate_Doc_Map(self->del_writer,
            deletions, SegReader_Doc_Max(seg_reader),
            Seg_Get_Count(self->segment) 
        );
        SegWriter_Merge_Segment(self->seg_writer, seg_reader, doc_map);
        DECREF(deletions);
        DECREF(doc_map);
    }

    self->optimized = true;
}

static CharBuf*
S_find_schema_file(Snapshot *snapshot)
{
    VArray *files = Snapshot_List(snapshot);
    u32_t i, max;
    CharBuf *retval = NULL;
    for (i = 0, max = VA_Get_Size(files); i < max; i++) {
        CharBuf *file = (CharBuf*)VA_Fetch(files, i);
        if (   CB_Starts_With_Str(file, "schema_", 7)
            && CB_Ends_With_Str(file, ".json", 5)
        ) {
            retval = file;
            break;
        }
    }
    DECREF(files);
    return retval;
}

void
Indexer_prepare_commit(Indexer *self)
{
    VArray *seg_readers     = PolyReader_Get_Seg_Readers(self->polyreader);
    u32_t   num_seg_readers = VA_Get_Size(seg_readers);
    bool_t  merge_happened  = self->optimized;

    if ( !self->write_lock || self->prepared ) {
        THROW("Can't call Prepare_Commit() more than once");
    }

    /* Merge existing index data. */
    if (num_seg_readers && !self->optimized) {
        VArray *to_merge = IxManager_SegReaders_To_Merge(self->manager,
            self->polyreader, false);
        u32_t i, max;

        /* Consolidate segments if either sparse or optimizing forced. */
        for (i = 0, max = VA_Get_Size(to_merge); i < max; i++) {
            SegReader *seg_reader = (SegReader*)VA_Fetch(to_merge, i);
            Matcher *deletions 
                = DelWriter_Seg_Deletions(self->del_writer, seg_reader);
            I32Array *doc_map = DelWriter_Generate_Doc_Map(self->del_writer,
                deletions, SegReader_Doc_Max(seg_reader),
                Seg_Get_Count(self->segment) 
            );
            SegWriter_Merge_Segment(self->seg_writer, seg_reader, doc_map);
            merge_happened = true;
            DECREF(deletions);
            DECREF(doc_map);
        }

        /* Write out new deletions. */
        if (DelWriter_Updated(self->del_writer)) {
            DelWriter_Finish(self->del_writer);
        }

        DECREF(to_merge);
    }

    /* Add a new segment and write a new snapshot file if... */
    if (   Seg_Get_Count(self->segment)      /* Docs/segs added. */
        || merge_happened                        /* Some segs merged. */
        || !Snapshot_Num_Entries(self->snapshot) /* Initializing index. */
        || DelWriter_Updated(self->del_writer) 
    ) {
        Folder   *folder   = self->folder;
        Schema   *schema   = self->schema;
        Snapshot *snapshot = self->snapshot;
        CharBuf  *seg_name = Seg_Get_Name(self->segment);
        CharBuf  *old_schema_name = S_find_schema_file(snapshot);
        i32_t     schema_gen = old_schema_name
                             ? IxFileNames_extract_gen(old_schema_name) + 1
                             : 1;
        CharBuf  *base36 = StrHelp_to_base36(schema_gen);
        CharBuf  *new_schema_name = CB_newf("schema_%o.json", base36);
        DECREF(base36);

        /* Finish the segment, write schema file. */
        SegWriter_Finish(self->seg_writer);
        Schema_Write(schema, folder, new_schema_name);
        Folder_Finish_Segment(folder, seg_name);
        if (old_schema_name) {
            Snapshot_Delete_Entry(snapshot, old_schema_name);
        }
        Snapshot_Add_Entry(snapshot, new_schema_name);
        DECREF(new_schema_name);

        /* Write temporary snapshot file. */
        DECREF(self->snapfile);
        self->snapfile = IxManager_Make_Snapshot_Filename(self->manager);
        CB_Cat_Trusted_Str(self->snapfile, ".temp", 5);
        Folder_Delete(folder, self->snapfile);
        Snapshot_Write_File(snapshot, folder, self->snapfile);

        self->needs_commit = true;
    }

    /* Close reader, so that we can delete its files if appropriate. */
    PolyReader_Close(self->polyreader);

    self->prepared = true;
}

void
Indexer_commit(Indexer *self)
{
    /* Safety check. */
    if ( !self->write_lock ) {
        THROW("Can't call commit() more than once");
    }

    if (!self->prepared) {
        Indexer_Prepare_Commit(self);
    }

    if (self->needs_commit) {
        /* Rename temp snapshot file. */
        CharBuf *temp_snapfile = CB_Clone(self->snapfile);
        CB_Chop(self->snapfile, sizeof(".temp") - 1);
        Snapshot_Set_Filename(self->snapshot, self->snapfile);
        Folder_Rename(self->folder, temp_snapfile, self->snapfile);
        DECREF(temp_snapfile);

        /* Purge obsolete files. */
        FilePurger_Purge(self->file_purger);
    }

    /* Release the write lock, invalidating the Indexer. */
    S_release_write_lock(self);
}

SegWriter*
Indexer_get_seg_writer(Indexer *self) { return self->seg_writer; }

static void
S_release_write_lock(Indexer *self)
{
    if (self->write_lock) {
        Lock_Release(self->write_lock);
        DECREF(self->write_lock);
        self->write_lock = NULL;
    }
}

/* Copyright 2005-2009 Marvin Humphrey
 *
 * This program is free software; you can redistribute it and/or modify
 * under the same terms as Perl itself.
 */

