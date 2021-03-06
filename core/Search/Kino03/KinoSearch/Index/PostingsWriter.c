#include "Search/Kino03/KinoSearch/Util/ToolSet.h"

#include "Search/Kino03/KinoSearch/Index/PostingsWriter.h"
#include "Search/Kino03/KinoSearch/Architecture.h"
#include "Search/Kino03/KinoSearch/Doc.h"
#include "Search/Kino03/KinoSearch/FieldType.h"
#include "Search/Kino03/KinoSearch/Posting.h"
#include "Search/Kino03/KinoSearch/Posting/RawPosting.h"
#include "Search/Kino03/KinoSearch/Schema.h"
#include "Search/Kino03/KinoSearch/Analysis/Inversion.h"
#include "Search/Kino03/KinoSearch/Index/Inverter.h"
#include "Search/Kino03/KinoSearch/Index/PolyReader.h"
#include "Search/Kino03/KinoSearch/Index/PostingPool.h"
#include "Search/Kino03/KinoSearch/Index/PostingPoolQueue.h"
#include "Search/Kino03/KinoSearch/Index/Segment.h"
#include "Search/Kino03/KinoSearch/Index/SkipStepper.h"
#include "Search/Kino03/KinoSearch/Index/SegReader.h"
#include "Search/Kino03/KinoSearch/Index/TermInfo.h"
#include "Search/Kino03/KinoSearch/Index/LexiconWriter.h"
#include "Search/Kino03/KinoSearch/Index/Snapshot.h"
#include "Search/Kino03/KinoSearch/Search/Similarity.h"
#include "Search/Kino03/KinoSearch/Store/Folder.h"
#include "Search/Kino03/KinoSearch/Store/InStream.h"
#include "Search/Kino03/KinoSearch/Store/OutStream.h"
#include "Search/Kino03/KinoSearch/Util/I32Array.h"
#include "Search/Kino03/KinoSearch/Util/MemoryPool.h"

i32_t PostWriter_current_file_format = 1;

/* Initialize a PostingPool for this field.  If this is the field's first
 * pool, create a VArray to hold this and subsequent pools.
 */
static void
S_init_posting_pool(PostingsWriter *self, const CharBuf *field);

/* Add an the content of an inverted field. */
static void
S_add_inversion(PostingsWriter *self, Inversion *inversion, 
                const CharBuf *field, i32_t doc_id, 
                float doc_boost, float length_norm);

/* Flush at least 75% of the acculumulated RAM cache to disk.
 */
static void 
S_flush_pools(PostingsWriter *self);

/* Write terms and postings to outstreams -- possibly temp, possibly real.
 */
static void
S_write_terms_and_postings(PostingsWriter *self, Obj *raw_post_source, 
                           OutStream *post_stream, OutStream *skip_stream);


PostingsWriter*
PostWriter_new(Snapshot *snapshot, Segment *segment, PolyReader *polyreader, 
               LexiconWriter *lex_writer)
{
    PostingsWriter *self 
        = (PostingsWriter*)VTable_Make_Obj(&POSTINGSWRITER);
    return PostWriter_init(self, snapshot, segment, polyreader, lex_writer);
}

#define DEFAULT_MEM_THRESH 0x1000000

PostingsWriter*
PostWriter_init(PostingsWriter *self, Snapshot *snapshot, Segment *segment,
                PolyReader *polyreader, LexiconWriter *lex_writer)
{
    Schema *schema = PolyReader_Get_Schema(polyreader);

    DataWriter_init((DataWriter*)self, snapshot, segment, polyreader);

    /* Assign. */
    self->lex_writer = (LexiconWriter*)INCREF(lex_writer);

    /* Init. */
    self->post_pools     = VA_new(Schema_Num_Fields(schema));
    self->skip_stream    = NULL;
    self->skip_stepper   = SkipStepper_new();
    self->mem_thresh     = DEFAULT_MEM_THRESH;
    self->mem_pool       = MemPool_new(DEFAULT_MEM_THRESH);
    self->lex_tempname   = CB_newf("%o/lextemp", Seg_Get_Name(segment));
    self->post_tempname  = CB_newf("%o/ptemp", Seg_Get_Name(segment));
    self->lex_outstream  = NULL;
    self->post_outstream = NULL;
    self->lex_instream   = NULL;
    self->post_instream  = NULL;

    return self;
}

static void
S_lazy_init(PostingsWriter *self)
{
    /* Open cache streams. */
    if (!self->lex_outstream) {
        Folder *folder = self->folder;
        self->lex_outstream  = Folder_Open_Out(folder, self->lex_tempname);
        self->post_outstream = Folder_Open_Out(folder, self->post_tempname);
        if (!self->lex_outstream) { 
            THROW("Can't open %o", self->lex_tempname); 
        }
        if (!self->post_outstream) { 
            THROW("Can't open %o", self->post_tempname); 
        }
    }
}

void
PostWriter_destroy(PostingsWriter *self)
{
    DECREF(self->lex_writer);
    DECREF(self->mem_pool);
    DECREF(self->post_pools);
    DECREF(self->skip_stepper);
    DECREF(self->lex_tempname);
    DECREF(self->post_tempname);
    DECREF(self->lex_outstream);
    DECREF(self->post_outstream);
    DECREF(self->lex_instream);
    DECREF(self->post_instream);
    DECREF(self->skip_stream);
    SUPER_DESTROY(self, POSTINGSWRITER);
}

i32_t
PostWriter_format(PostingsWriter *self)
{
    UNUSED_VAR(self);
    return PostWriter_current_file_format;
}

static void
S_init_posting_pool(PostingsWriter *self, const CharBuf *field)
{
    Schema *schema           = self->schema;
    i32_t   field_num        = Seg_Field_Num(self->segment, field);
    VArray *field_post_pools = (VArray*)VA_Fetch(self->post_pools, field_num);
    PostingPool *post_pool = PostPool_new(schema, field, self->mem_pool);

    if (field_post_pools == NULL) {
        field_post_pools = VA_new(1);
        VA_Store(self->post_pools, field_num, (Obj*)field_post_pools);
    }

    /* Make sure the first pool in the array always has space. */
    VA_Unshift(field_post_pools, (Obj*)post_pool);
}

void
PostWriter_add_inverted_doc(PostingsWriter *self, Inverter *inverter, 
                            i32_t doc_id)
{
    float doc_boost = Inverter_Get_Boost(inverter);
    S_lazy_init(self);

    Inverter_Iter_Init(inverter);
    while (Inverter_Next(inverter)) {
        FieldType *type = Inverter_Get_Type(inverter);
        if (type->indexed) {
            CharBuf     *field       = Inverter_Get_Field_Name(inverter);
            Similarity  *sim         = Inverter_Get_Similarity(inverter);
            Inversion   *inversion   = Inverter_Get_Inversion(inverter);
            float        length_norm = Sim_Length_Norm(sim, inversion->size);
            S_add_inversion(self, inversion, (CharBuf*)field, doc_id,
                doc_boost, length_norm);
        }
    }
}

static void
S_add_inversion(PostingsWriter *self, Inversion *inversion, 
                const CharBuf *field, i32_t doc_id, 
                float doc_boost, float length_norm)
{
    i32_t         field_num  = Seg_Field_Num(self->segment, field);
    PostingPool  *post_pool;
    VArray       *field_post_pools;
    
    /* Retrieve the current PostingPool for this field. */
    field_post_pools = (VArray*)VA_Fetch(self->post_pools, field_num);
    if (field_post_pools == NULL) {
        S_init_posting_pool(self, field);
        field_post_pools = (VArray*)VA_Fetch(self->post_pools, field_num);
    }
    post_pool = (PostingPool*)VA_Fetch(field_post_pools, 0);

    /* Add the Inversion to the PostingPool. */
    PostPool_Add_Inversion(post_pool, inversion, doc_id, doc_boost, 
        length_norm);

    /* Check if we've crossed the memory threshold and it's time to flush. */
    if (self->mem_pool->consumed > self->mem_thresh)
        S_flush_pools(self);
}

static void 
S_flush_pools(PostingsWriter *self)
{
    u32_t i, max;
    VArray *const post_pools = self->post_pools;

    for (i = 0, max = VA_Get_Size(post_pools); i < max; i++) {
        VArray *field_post_pools = (VArray*)VA_Fetch(post_pools, i);
        if (field_post_pools != NULL) {
            /* The first pool in the array is the only active pool. */
            PostingPool *const post_pool 
                = (PostingPool*)VA_Fetch(field_post_pools, 0);

            if (post_pool->cache_max != post_pool->cache_tick) {
                /* Open a skip stream if it hasn't been already. */
                if (self->skip_stream == NULL) {
                    Snapshot *snapshot = PostWriter_Get_Snapshot(self);
                    CharBuf *filename = CB_newf("%o/postings.skip", 
                        Seg_Get_Name(self->segment));
                    self->skip_stream 
                        = Folder_Open_Out(self->folder, filename);
                    if (!self->skip_stream) {
                        THROW("Can't open %o", filename);
                    }
                    Snapshot_Add_Entry(snapshot, filename);
                    DECREF(filename);
                }

                /* Write to temp files. */
                LexWriter_Enter_Temp_Mode(self->lex_writer, 
                    self->lex_outstream);
                post_pool->lex_start = OutStream_Tell(self->lex_outstream);
                post_pool->post_start = OutStream_Tell(self->post_outstream);
                PostPool_Sort_Cache(post_pool);
                S_write_terms_and_postings(self, (Obj*)post_pool, 
                    self->post_outstream, self->skip_stream);
                post_pool->lex_end = OutStream_Tell(self->lex_outstream);
                post_pool->post_end = OutStream_Tell(self->post_outstream);
                LexWriter_Leave_Temp_Mode(self->lex_writer);

                /* Store away this pool and start another. */
                PostPool_Shrink(post_pool);
                S_init_posting_pool(self, post_pool->field);
            }
        }
    }

    /* Now that we've flushed all RawPostings, release memory. */
    MemPool_Release_All(self->mem_pool);
}

typedef RawPosting*
(*fetcher_t)(Obj *raw_post_source);

static void
S_write_terms_and_postings(PostingsWriter *self, Obj *raw_post_source, 
                         OutStream *post_stream, OutStream *skip_stream)
{
    TermInfo         *const tinfo           = TInfo_new(0,0,0,0);
    SkipStepper      *const skip_stepper    = self->skip_stepper;
    LexiconWriter    *const lex_writer      = self->lex_writer;
    const i32_t       skip_interval         = lex_writer->skip_interval;
    CharBuf          *const last_term_text  = CB_new(0);
    i32_t             last_doc_id           = 0;
    i32_t             last_skip_doc         = 0;
    u64_t             last_skip_filepos     = 0;
    RawPosting       *posting               = NULL;
    fetcher_t         fetch                 = NULL;

    /* Cache fetch method (violates OO principles, but we'll deal). */
    if (OBJ_IS_A(raw_post_source, POSTINGPOOL)) {
        fetch = (fetcher_t)METHOD(raw_post_source->vtable, PostPool,
            Fetch_From_RAM);
    }
    else if (OBJ_IS_A(raw_post_source, POSTINGPOOLQUEUE)) {
        fetch = (fetcher_t)METHOD(raw_post_source->vtable, PostPoolQ, 
            Fetch);
    }

    /* Prime heldover variables. */
    SkipStepper_Reset(skip_stepper, 0, 0);
    posting = fetch(raw_post_source);
    if (posting == NULL)
        THROW("Failed to retrieve at least one posting");
    CB_Copy_Str(last_term_text, posting->blob, posting->content_len);

    while (1) {
        bool_t same_text_as_last = true;

        if (posting == NULL) {
            /* On the last iter, use an empty string to make 
             * LexiconWriter DTRT. */
            posting = &RAWPOSTING_BLANK;
            same_text_as_last = false;
        }
        else {
            /* Compare once. */
            if (   posting->content_len != CB_Get_Size(last_term_text)
                || memcmp(&posting->blob, last_term_text->ptr, 
                    posting->content_len) != 0
            ) {
                same_text_as_last = false;
            }
        }

        /* If the term text changes, process the last term. */
        if ( !same_text_as_last ) {
            /* Take note of where we are for the term dictionary. */
            u64_t post_filepos = OutStream_Tell(post_stream);

            /* Hand off to LexiconWriter. */
            LexWriter_Add_Term(lex_writer, last_term_text, tinfo);

            /* Start each term afresh. */
            tinfo->doc_freq      = 0;
            tinfo->post_filepos  = post_filepos;
            tinfo->skip_filepos  = 0;
            tinfo->lex_filepos   = 0;

            /* Init skip data in preparation for the next term. */
            skip_stepper->doc_id  = 0;
            skip_stepper->filepos = post_filepos;
            last_skip_doc         = 0;
            last_skip_filepos     = post_filepos;

            /* Remember the term_text so we can write string diffs. */
            CB_Copy_Str(last_term_text, posting->blob, 
                posting->content_len);

            /* Starting a new term, thus a new delta doc sequence at 0. */
            last_doc_id = 0;
        }

        /* Bail on last iter before writing invalid posting data. */
        if (posting == &RAWPOSTING_BLANK)
            break;

        /* Write posting data. */
        RawPost_Write_Record(posting, post_stream, last_doc_id);

        /* Doc freq lags by one iter. */
        tinfo->doc_freq++;

        /*  Write skip data. */
        if (   skip_stream != NULL
            && same_text_as_last   
            && tinfo->doc_freq % skip_interval == 0
            && tinfo->doc_freq != 0
        ) {
            /* If first skip group, save skip stream pos for term info. */
            if (tinfo->doc_freq == skip_interval) {
                tinfo->skip_filepos = OutStream_Tell(skip_stream); 
            }
            /* Write deltas. */
            last_skip_doc         = skip_stepper->doc_id;
            last_skip_filepos     = skip_stepper->filepos;
            skip_stepper->doc_id  = posting->doc_id;
            skip_stepper->filepos = OutStream_Tell(post_stream);
            SkipStepper_Write_Record(skip_stepper, skip_stream,
                 last_skip_doc, last_skip_filepos);
        }

        /* Remember last doc id because we need it for delta encoding. */
        last_doc_id = posting->doc_id;

        /* Retrieve the next posting from the sort pool. */
        /* DECREF(posting); */ /* No!!  DON'T destroy!!!  */
        posting = fetch(raw_post_source);

    }

    /* Clean up. */
    DECREF(last_term_text);
    DECREF(tinfo);
}

static void
S_finish_field(PostingsWriter *self, i32_t field_num)
{
    VArray *field_post_pools = (VArray*)VA_Delete(self->post_pools, field_num);
    PostingPoolQueue *pool_q;
    
    if (field_post_pools == NULL)
        return;

    /* TODO: can reusing mem_thresh double ram footprint? */
    pool_q = PostPoolQ_new(field_post_pools, self->lex_instream,
        self->post_instream, self->mem_thresh); 
    DECREF(field_post_pools);

    /* Don't bother unless there's actually content. */
    if (PostPoolQ_Peek(pool_q) != NULL) {
        Snapshot         *snapshot      = PostWriter_Get_Snapshot(self);
        LexiconWriter    *lex_writer    = self->lex_writer;
        Folder           *folder        = self->folder;
        OutStream        *post_stream   = NULL;
        OutStream        *skip_stream   = self->skip_stream;
        CharBuf          *filename      = CB_newf("%o/postings-%i32.dat", 
            Seg_Get_Name(self->segment), field_num);

        /* Open posting stream. */
        Snapshot_Add_Entry(snapshot, filename);
        post_stream = Folder_Open_Out(folder, filename);
        if (!post_stream) { THROW("Can't open %o", filename); }
    
        /* Open a skip stream if it hasn't been already. */
        if (self->skip_stream == NULL) {
            CharBuf *skip_filename = CB_newf("%o/postings.skip", 
                Seg_Get_Name(self->segment));
            Snapshot_Add_Entry(snapshot, skip_filename);
            skip_stream = Folder_Open_Out(folder, skip_filename);
            if (!skip_stream) { THROW("Can't open %o", skip_filename); }
            self->skip_stream = skip_stream;
            DECREF(skip_filename);
        }

        /* Start LexiconWriter. */
        LexWriter_Start_Field(lex_writer, field_num);

        /* Write terms and postings. */
        S_write_terms_and_postings(self, (Obj*)pool_q, post_stream, 
            skip_stream);

        /* Finish and clean up. */
        LexWriter_Finish_Field(self->lex_writer, field_num);
        OutStream_Close(post_stream);
        DECREF(filename);
        DECREF(post_stream);
    }

    /* Clean up. */
    DECREF(pool_q);
}

void
PostWriter_add_segment(PostingsWriter *self, SegReader *reader,
                       I32Array *doc_map)
{
    Folder    *other_folder   = SegReader_Get_Folder(reader);
    Segment   *other_segment  = reader->segment;
    u32_t      i, max;
    VArray    *post_pools     = self->post_pools;
    Schema    *schema         = self->schema;
    Segment   *segment        = self->segment;
    VArray    *all_fields     = Schema_All_Fields(schema);
    S_lazy_init(self);

    for (i = 0, max = VA_Get_Size(all_fields); i < max; i++) {
        CharBuf   *field    = (CharBuf*)VA_Fetch(all_fields, i);
        FieldType *type     = Schema_Fetch_Type(schema, field);
        i32_t old_field_num = Seg_Field_Num(other_segment, field);
        i32_t new_field_num = Seg_Field_Num(segment, field);
        VArray *field_post_pools   = NULL;

        if (!FType_Indexed(type)) { continue; }
        if (!old_field_num) { continue; } /* not in old segment */
        if (!new_field_num) { THROW("Unrecognized field: %o", field); }

        /* Init field if we've never seen it before. */
        field_post_pools = (VArray*)VA_Fetch(post_pools, new_field_num);
        if (field_post_pools == NULL) {
            S_init_posting_pool(self, field);
            field_post_pools 
                = (VArray*)VA_Fetch(self->post_pools, new_field_num);
        }

        /* Create a pool and add it to the field's collection of pools. */
        { 
            PostingPool *post_pool
                = PostPool_new(schema, field, self->mem_pool);
            PostPool_Assign_Seg(post_pool, other_folder, other_segment, 
                Seg_Get_Count(segment), doc_map);
            VA_Push(field_post_pools, (Obj*)post_pool);
        }
    }

    /* Clean up. */
    DECREF(all_fields);
}

static bool_t 
S_my_file(VArray *array, u32_t tick, void *data)
{
    CharBuf *file_start = (CharBuf*)data;
    CharBuf *candidate = (CharBuf*)VA_Fetch(array, tick);
    return CB_Starts_With(candidate, file_start);
}

void
PostWriter_delete_segment(PostingsWriter *self, SegReader *reader)
{
    Snapshot *snapshot = PostWriter_Get_Snapshot(self);
    CharBuf  *merged_seg_name = Seg_Get_Name(SegReader_Get_Segment(reader));
    CharBuf  *pattern  = CB_newf("%o/postings", merged_seg_name);
    VArray   *files = Snapshot_List(snapshot);
    VArray   *my_old_files = VA_Grep(files, S_my_file, pattern);
    u32_t i, max;

    /* Delete my files. */
    for (i = 0, max = VA_Get_Size(my_old_files); i < max; i++) {
        CharBuf *entry = (CharBuf*)VA_Fetch(my_old_files, i);
        Snapshot_Delete_Entry(snapshot, entry);
    }
    DECREF(my_old_files);
    DECREF(files);
    DECREF(pattern);

    /* Delete lexicon files. */
    LexWriter_Delete_Segment(self->lex_writer, reader);
}

void
PostWriter_finish(PostingsWriter *self)
{
    if (self->lex_outstream) { /* S_lazy_init was called */
        Folder *folder = self->folder;
        u32_t i, max;

        /* Switch temp streams from write to read mode. */
        OutStream_Close(self->lex_outstream);
        OutStream_Close(self->post_outstream);
        self->lex_instream  = Folder_Open_In(folder, self->lex_tempname);
        self->post_instream = Folder_Open_In(folder, self->post_tempname);
        if (!self->lex_instream) { 
            THROW("Can't open %o", self->lex_tempname); 
        }
        if (!self->post_instream) { 
            THROW("Can't open %o", self->post_tempname); 
        }

        /* Write postings for each field. */
        for (i = 0, max = VA_Get_Size(self->post_pools); i < max; i++) {
            S_finish_field(self, i);
        }

        /* Close down. */
        if (self->skip_stream != NULL)
            OutStream_Close(self->skip_stream);
        
        /* Store metadata. */
        Seg_Store_Metadata_Str(self->segment, "postings", 8, 
            (Obj*)PostWriter_Metadata(self));

        /* Clean up. */
        InStream_Close(self->lex_instream);
        InStream_Close(self->post_instream);
        DECREF(self->lex_instream);
        DECREF(self->post_instream);
        self->lex_instream = NULL;
        self->post_instream = NULL;
        if (!Folder_Delete(folder, self->lex_tempname)) {
            THROW("Couldn't delete %o", self->lex_tempname);
        }
        if (!Folder_Delete(folder, self->post_tempname)) {
            THROW("Couldn't delete %o", self->post_tempname);
        }
    }

    /* Dispatch the LexiconWriter. */
    LexWriter_Finish(self->lex_writer);
}

/* Copyright 2006-2009 Marvin Humphrey
 *
 * This program is free software; you can redistribute it and/or modify
 * under the same terms as Perl itself.
 */

