#include "Search/Kino03/KinoSearch/Util/ToolSet.h"

#include "Search/Kino03/KinoSearch/Index/PolyReader.h"
#include "Search/Kino03/KinoSearch/Doc/HitDoc.h"
#include "Search/Kino03/KinoSearch/FieldType.h"
#include "Search/Kino03/KinoSearch/Schema.h"
#include "Search/Kino03/KinoSearch/Index/DeletionsReader.h"
#include "Search/Kino03/KinoSearch/Index/Segment.h"
#include "Search/Kino03/KinoSearch/Index/SegReader.h"
#include "Search/Kino03/KinoSearch/Index/Snapshot.h"
#include "Search/Kino03/KinoSearch/Store/Folder.h"
#include "Search/Kino03/KinoSearch/Store/FSFolder.h"
#include "Search/Kino03/KinoSearch/Store/Lock.h"
#include "Search/Kino03/KinoSearch/Store/LockFactory.h"
#include "Search/Kino03/KinoSearch/Util/I32Array.h"
#include "Search/Kino03/KinoSearch/Util/Json.h"
#include "Search/Kino03/KinoSearch/Util/IndexFileNames.h"
#include "Search/Kino03/KinoSearch/Util/StringHelper.h"

/* Obtain/release read locks and commit locks.  If self->lock_factory is
 * NULL, do nothing  */
static void 
S_obtain_read_lock(PolyReader *self, const CharBuf *snapshot_filename);
static void 
S_obtain_commit_lock(PolyReader *self);
static void 
S_release_read_lock(PolyReader *self);
static void 
S_release_commit_lock(PolyReader *self);

static Folder*
S_derive_folder(Obj *index);

PolyReader*
PolyReader_new(Schema *schema, Folder *folder, Snapshot *snapshot, 
               LockFactory *lock_factory, VArray *sub_readers)
{
    PolyReader *self = (PolyReader*)VTable_Make_Obj(&POLYREADER);
    return PolyReader_init(self, schema, folder, snapshot, lock_factory, 
        sub_readers);
}

PolyReader*
PolyReader_open(Obj *index, Snapshot *snapshot, LockFactory *lock_factory)
{
    PolyReader *self = (PolyReader*)VTable_Make_Obj(&POLYREADER);
    return PolyReader_do_open(self, index, snapshot, lock_factory);
}

static Obj*
S_first_non_null(VArray *array)
{
    u32_t i, max;
    for (i = 0, max = VA_Get_Size(array); i < max; i++) {
        Obj *thing = VA_Fetch(array, i);
        if (thing) { return thing; }
    }
    return NULL;
}

static void
S_init_sub_readers(PolyReader *self, VArray *sub_readers) 
{
    u32_t  i;
    u32_t  num_sub_readers = VA_Get_Size(sub_readers);
    i32_t *starts = MALLOCATE(num_sub_readers, i32_t);
    Hash  *data_readers = Hash_new(0);

    DECREF(self->sub_readers);
    DECREF(self->offsets);
    self->sub_readers       = (VArray*)INCREF(sub_readers);

    /* Accumulate doc_max, subreader start offsets, and DataReaders. */
    self->doc_max = 0;
    for (i = 0; i < num_sub_readers; i++) {
        SegReader *seg_reader = (SegReader*)VA_Fetch(sub_readers, i);
        Hash *components = SegReader_Get_Components(seg_reader);
        CharBuf *interface;
        DataReader *component;
        starts[i] = self->doc_max;
        self->doc_max += SegReader_Doc_Max(seg_reader);
        Hash_Iter_Init(components);
        while (Hash_Iter_Next(components, &interface, (Obj**)&component)) {
            VArray *readers 
                = (VArray*)Hash_Fetch(data_readers, interface);
            if (!readers) { 
                readers = VA_new(num_sub_readers); 
                Hash_Store(data_readers, interface, (Obj*)readers);
            }
            VA_Store(readers, i, INCREF(component));
        }
    }
    self->offsets = I32Arr_new_steal(starts, num_sub_readers);

    {
        CharBuf *interface;
        VArray  *readers;
        Hash_Iter_Init(data_readers);
        while (Hash_Iter_Next(data_readers, &interface, (Obj**)&readers)) {
            DataReader *datareader = (DataReader*)ASSERT_IS_A(
                S_first_non_null(readers), DATAREADER);
            DataReader *aggregator 
                = DataReader_Aggregator(datareader, readers, self->offsets);
            if (aggregator) {
                ASSERT_IS_A(aggregator, DATAREADER);
                Hash_Store(self->components, interface, (Obj*)aggregator);
            }
        }
    }
    DECREF(data_readers);

    {
        DeletionsReader *del_reader = (DeletionsReader*)Hash_Fetch(
            self->components, DELETIONSREADER.name);
        self->del_count = del_reader ? DelReader_Del_Count(del_reader) : 0;
    }
}

PolyReader*
PolyReader_init(PolyReader *self, Schema *schema, Folder *folder, 
                Snapshot *snapshot, LockFactory *lock_factory, 
                VArray *sub_readers)
{
    self->doc_max    = 0;
    self->del_count  = 0;

    if (sub_readers) { 
        u32_t num_segs = VA_Get_Size(sub_readers);
        VArray *segments = VA_new(num_segs);
        u32_t i;
        for (i = 0; i < num_segs; i++) {
            SegReader *seg_reader = (SegReader*)ASSERT_IS_A(
                VA_Fetch(sub_readers, i), SEGREADER);
            VA_Push(segments, INCREF(SegReader_Get_Segment(seg_reader)));
        }
        IxReader_init((IndexReader*)self, schema, folder, snapshot, 
            segments, -1, lock_factory);
        DECREF(segments);
        S_init_sub_readers(self, sub_readers); 
    }
    else {
        IxReader_init((IndexReader*)self, schema, folder, snapshot, 
            NULL, -1, lock_factory);
        self->sub_readers = VA_new(0);
        self->offsets = I32Arr_new_steal(NULL, 0);
    }

    return self;
}

void
PolyReader_close(PolyReader *self)
{
    PolyReader_close_t super_close 
        = (PolyReader_close_t)SUPER_METHOD(&POLYREADER, PolyReader, Close);
    u32_t i, max;
    for (i = 0, max = VA_Get_Size(self->sub_readers); i < max; i++) {
        SegReader *seg_reader = (SegReader*)VA_Fetch(self->sub_readers, i);
        SegReader_Close(seg_reader); 
    }
    super_close(self);
}

void
PolyReader_destroy(PolyReader *self)
{
    DECREF(self->sub_readers);
    DECREF(self->offsets);
    SUPER_DESTROY(self, POLYREADER);
}

Obj*
S_try_open_elements(PolyReader *self)
{
    VArray  *files             = Snapshot_List(self->snapshot);
    Folder  *folder            = PolyReader_Get_Folder(self);
    u32_t    num_segs          = 0;
    i32_t    latest_schema_gen = 0;
    CharBuf *schema_file       = NULL;
    VArray  *segments;
    u32_t    i, max;

    /* Find schema file, count segments. */
    for (i = 0, max = VA_Get_Size(files); i < max; i++) {
        CharBuf *file = (CharBuf*)VA_Fetch(files, i);

        if (CB_Ends_With_Str(file, "segmeta.json", 12)) {
            num_segs++;
        }
        else if (   CB_Starts_With_Str(file, "schema_", 7)
                 && CB_Ends_With_Str(file, ".json", 5)
        ) {
            i32_t gen = IxFileNames_extract_gen(file);
            if (gen > latest_schema_gen) {
                latest_schema_gen = gen;
                if (!schema_file) { schema_file = CB_Clone(file); }
                else { CB_Copy(schema_file, file); }
            }
        }
    }

    /* Read Schema. */
    if (!schema_file) {
        CharBuf *mess = MAKE_MESS("Can't find a schema file.");
        DECREF(files);
        return (Obj*)mess;
    }
    else {
        Hash *dump = (Hash*)Json_slurp_json(folder, schema_file);
        if (dump) { /* read file successfully */
            DECREF(self->schema);
            self->schema = (Schema*)ASSERT_IS_A(
                VTable_Load_Obj(&SCHEMA, (Obj*)dump), SCHEMA);
            DECREF(dump);
            DECREF(schema_file);
            schema_file = NULL;
        }
        else {
            CharBuf *mess = MAKE_MESS("Failed to parse %o", schema_file);
            DECREF(schema_file);
            DECREF(files);
            return (Obj*)mess;
        }
    }

    segments = VA_new(num_segs);
    for (i = 0, max = VA_Get_Size(files); i < max; i++) {
        CharBuf *file = (CharBuf*)VA_Fetch(files, i);

        /* Create a Segment for each segmeta. */
        if (CB_Ends_With_Str(file, "segmeta.json", 12)) {
            ZombieCharBuf seg_name = ZCB_make(file);
            Segment *segment;
            ZCB_Chop(&seg_name, sizeof("/segmeta.json") - 1);
            segment = Seg_new((CharBuf*)&seg_name, folder);

            /* Bail if reading the file fails (probably because it's been
             * deleted and a new snapshot file has been written so we need to
             * retry). */
            if (Seg_Read_File(segment)) {
                VA_Push(segments, (Obj*)segment);
            }
            else {
                CharBuf *mess = MAKE_MESS("Failed to read %o", file);
                DECREF(segment);
                DECREF(segments);
                DECREF(files);
                return (Obj*)mess;
            }
        }
    }

    /* Sort the segments by age. */
    VA_Sort(segments, NULL);

    {
        Obj *result = PolyReader_Try_Open_SegReaders(self, segments);
        DECREF(segments);
        DECREF(files);
        return result;
    }
}

/* For test suite. */
CharBuf* PolyReader_race_condition_debug1 = NULL;
i32_t    PolyReader_debug1_num_passes     = 0;

PolyReader*
PolyReader_do_open(PolyReader *self, Obj *index, Snapshot *snapshot, 
                   LockFactory *lock_factory)
{
    Folder *folder = S_derive_folder(index);
    i32_t last_gen = 0;

    PolyReader_init(self, NULL, folder, snapshot, lock_factory, NULL);
    DECREF(folder);

    if (lock_factory) S_obtain_commit_lock(self);

    while (1) {
        CharBuf *target_snap_file;
        i32_t gen;

        /* If a Snapshot was supplied, use its file. */
        if (snapshot) {
            target_snap_file = Snapshot_Get_Filename(snapshot);
            if (!target_snap_file) {
                THROW("Supplied snapshot objects must not be empty");
            }
            else {
                CB_Inc_RefCount(target_snap_file);
            }
        }
        else {
            /* Otherwise, pick the most recent snap file. */
            target_snap_file = IxFileNames_latest_snapshot(folder);

            /* No snap file?  Looks like the index is empty.  We can stop now and
             * return NULL. */
            if (!target_snap_file) { break; }
        }
        
        /* Derive "generation" of this snapshot file from its name. */
        gen = IxFileNames_extract_gen(target_snap_file);

        /* Get a read lock on the most recent snapshot file if indicated. */
        if (lock_factory) {
            S_obtain_read_lock(self, target_snap_file);
        }

        /* Testing only. */
        if (PolyReader_race_condition_debug1) {
            ZombieCharBuf temp = ZCB_LITERAL("temp");
            if (Folder_Exists(folder, (CharBuf*)&temp)) {
                Folder_Rename(folder, (CharBuf*)&temp,
                    PolyReader_race_condition_debug1);
            }
            PolyReader_debug1_num_passes++;
        }

        /* If a Snapshot object was passed in, the file has already been read.
         * If that's not the case, we need to go read the file we just picked. */
        if (!snapshot) { 
            CharBuf *error = PolyReader_try_read_snapshot(self->snapshot, 
                folder, target_snap_file);

            if (error) {
                S_release_read_lock(self);
                DECREF(target_snap_file);
                if (last_gen < gen) { /* Index updated, so try again. */
                    DECREF(error);
                    last_gen = gen;
                    continue;
                }
                else { /* Real error. */
                    if (lock_factory) S_release_commit_lock(self);
                    Err_throw_mess(error);
                }
            }
        }

        /* It's possible, though unlikely, for an Indexer to delete files
         * out from underneath us after the snapshot file is read but before
         * we've got SegReaders holding open all the required files.  If we
         * failed to open something, see if we can find a newer snapshot file.
         * If we can, then the exception was due to the race condition.  If
         * not, we have a real exception, so throw an error. */
        {
            Obj *result = S_try_open_elements(self);
            if (OBJ_IS_A(result, CHARBUF)) { /* Error occurred. */
                S_release_read_lock(self);
                DECREF(target_snap_file);
                if (last_gen < gen) { /* Index updated, so try again. */
                    DECREF(result);
                    last_gen = gen;
                }
                else { /* Real error. */
                    if (lock_factory) S_release_commit_lock(self);
                    Err_throw_mess((CharBuf*)result);
                }
            }
            else { /* Succeeded. */
                S_init_sub_readers(self, (VArray*)result);
                DECREF(result);
                DECREF(target_snap_file);
                break;
            }
        }
    }

    if (lock_factory) { S_release_commit_lock(self); }

    return self;
}

static Folder*
S_derive_folder(Obj *index)
{
    Folder *folder = NULL;
    if (OBJ_IS_A(index, FOLDER)) {
        folder = (Folder*)INCREF(index);
    }
    else if (OBJ_IS_A(index, CHARBUF)) {
        folder = (Folder*)FSFolder_new((CharBuf*)index);
    }
    else {
        THROW("Invalid type for 'index': %o", Obj_Get_Class_Name(index));
    }
    return folder;
}

static void
S_obtain_commit_lock(PolyReader *self)
{
    self->commit_lock = LockFact_Make_Lock(self->lock_factory,
        (CharBuf*)Lock_commit_lock_name, Lock_commit_lock_timeout);
    Lock_Clear_Stale(self->commit_lock);
    if (!Lock_Obtain(self->commit_lock)) {
        DECREF(self->commit_lock);
        self->commit_lock = NULL;
        THROW("Couldn't get commit lock"); 
    }
}

static void
S_obtain_read_lock(PolyReader *self, const CharBuf *snapshot_file_name)
{
    ZombieCharBuf lock_name = ZCB_BLANK;

    if (!self->lock_factory) return;
    if (   !CB_Starts_With_Str(snapshot_file_name, "snapshot_", 9)
        || !CB_Ends_With_Str(snapshot_file_name, ".json", 5)
    ) {
        THROW("Not a snapshot filename: %o", snapshot_file_name);
    }
        
    /* Truncate ".json" from end of snapshot file name. */
    ZCB_Assign(&lock_name, snapshot_file_name);
    ZCB_Chop(&lock_name, 5);

    self->read_lock = LockFact_Make_Shared_Lock(self->lock_factory,
        (CharBuf*)&lock_name, Lock_read_lock_timeout);

    Lock_Clear_Stale(self->read_lock);
    if (!Lock_Obtain(self->read_lock)) {
        DECREF(self->read_lock);
        THROW("Couldn't get read lock for %o", snapshot_file_name); 
    }
}

static void
S_release_read_lock(PolyReader *self)
{
    if (self->read_lock) {
        Lock_Release(self->read_lock);
        DECREF(self->read_lock);
        self->read_lock = NULL;
    }
}

static void
S_release_commit_lock(PolyReader *self)
{
    if (self->commit_lock) {
        Lock_Release(self->commit_lock);
        DECREF(self->commit_lock);
        self->commit_lock = NULL;
    }
}

i32_t 
PolyReader_doc_max(PolyReader *self)
{
    return self->doc_max;
}

i32_t
PolyReader_doc_count(PolyReader *self)
{
    return self->doc_max - self->del_count;
}

i32_t
PolyReader_del_count(PolyReader *self)
{
    return self->del_count;
}

I32Array*
PolyReader_offsets(PolyReader *self) 
{
    return (I32Array*)INCREF(self->offsets);
}

VArray*
PolyReader_seg_readers(PolyReader *self)
{
    return (VArray*)VA_Shallow_Copy(self->sub_readers);
}

VArray*
PolyReader_get_seg_readers(PolyReader *self) { return self->sub_readers; }

u32_t
PolyReader_sub_tick(I32Array *offsets, i32_t doc_id)
{
    u32_t lo = 0;
    u32_t hi_tick = I32Arr_Get_Size(offsets) - 1;
    u32_t hi = hi_tick;
    
    while (hi >= lo) {
        u32_t mid = lo + ((hi - lo) / 2);
        i32_t mid_start = I32Arr_Get(offsets, mid) + 1;
        if (doc_id < mid_start) {
            hi = mid - 1;
        }
        else if (doc_id > mid_start) {
            lo = mid + 1;
        }
        else {
            while (   mid < hi_tick 
                   && I32Arr_Get(offsets, mid + 1) == mid_start
            ) {
                mid++;
            }
            return mid;
        }
    }
    return hi;
}
    
/* Copyright 2006-2009 Marvin Humphrey
 *
 * This program is free software; you can redistribute it and/or modify
 * under the same terms as Perl itself.
 */

