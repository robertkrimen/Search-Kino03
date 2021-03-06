#include "Search/Kino03/KinoSearch/Util/ToolSet.h"

#include "Search/Kino03/KinoSearch/Index/LexIndex.h"
#include "Search/Kino03/KinoSearch/Architecture.h"
#include "Search/Kino03/KinoSearch/FieldType.h"
#include "Search/Kino03/KinoSearch/Index/Segment.h"
#include "Search/Kino03/KinoSearch/Index/TermInfo.h"
#include "Search/Kino03/KinoSearch/Schema.h"
#include "Search/Kino03/KinoSearch/Store/Folder.h"
#include "Search/Kino03/KinoSearch/Store/InStream.h"

/* Read the data we've arrived at after a seek operation. */
static void
S_read_entry(LexIndex *self);

LexIndex*
LexIndex_new(Schema *schema, Folder *folder, Segment *segment, 
             const CharBuf *field)
{
    LexIndex *self = (LexIndex*)VTable_Make_Obj(&LEXINDEX);
    return LexIndex_init(self, schema, folder, segment, field);
}

LexIndex*
LexIndex_init(LexIndex *self, Schema *schema, Folder *folder, 
              Segment *segment, const CharBuf *field)
{
    i32_t    field_num = Seg_Field_Num(segment, field);
    CharBuf *seg_name  = Seg_Get_Name(segment);
    CharBuf *ixix_file = CB_newf("%o/lexicon-%i32.ixix", seg_name, field_num);
    CharBuf *ix_file   = CB_newf("%o/lexicon-%i32.ix", seg_name, field_num);
    Architecture *arch = Schema_Get_Architecture(schema);

    /* Init. */
    self->term  = ViewCB_new_from_trusted_utf8(NULL, 0);
    self->tinfo = TInfo_new(0,0,0,0);
    self->tick  = 0;

    /* Derive */
    self->field_type = Schema_Fetch_Type(schema, field);
    if (!self->field_type) {
        CharBuf *mess = MAKE_MESS("Unknown field: '%o'", field);
        DECREF(ix_file);
        DECREF(ixix_file);
        DECREF(self);
        Err_throw_mess(mess);
    }
    INCREF(self->field_type);
    self->ixix_in = Folder_Open_In(folder, ixix_file);
    self->ix_in   = Folder_Open_In(folder, ix_file);
    if (!self->ixix_in || !self->ix_in) {
        CharBuf *mess =
             MAKE_MESS("Can't open either %o or %o", ix_file, ixix_file);
        DECREF(ix_file);
        DECREF(ixix_file);
        DECREF(self);
        Err_throw_mess(mess);
    }
    self->index_interval = Arch_Index_Interval(arch);
    self->skip_interval  = Arch_Skip_Interval(arch);
    self->size    = (i32_t)(InStream_Length(self->ixix_in) / sizeof(i64_t));
    self->offsets = (i64_t*)InStream_Buf(self->ixix_in,
        (size_t)InStream_Length(self->ixix_in));
    self->data = InStream_Buf(self->ix_in, InStream_Length(self->ix_in));
    self->limit = self->data + InStream_Length(self->ix_in);

    DECREF(ixix_file);
    DECREF(ix_file);

    return self;
}

void
LexIndex_destroy(LexIndex *self) 
{    
    DECREF(self->field_type);
    DECREF(self->ixix_in);
    DECREF(self->ix_in);
    DECREF(self->term);
    DECREF(self->tinfo);
    FREE_OBJ(self);
}

i32_t
LexIndex_get_term_num(LexIndex *self)
{
    return (self->index_interval * self->tick) - 1;
}

Obj*
LexIndex_get_term(LexIndex *self) { return (Obj*)self->term; }
TermInfo*
LexIndex_get_term_info(LexIndex *self) { return self->tinfo; }

static void
S_read_entry(LexIndex *self)
{
    TermInfo *tinfo  = self->tinfo;
    i64_t     offset = (i64_t)Math_decode_bigend_u64(self->offsets + self->tick);
    char     *data   = self->data + offset;
    size_t    size   = Math_decode_c32(&data);

    ViewCB_Assign_Str(self->term, data, size);
    data += size;
    tinfo->doc_freq = Math_decode_c32(&data);
    tinfo->post_filepos = Math_decode_c64(&data);
    tinfo->skip_filepos = tinfo->doc_freq >= self->skip_interval
                        ? Math_decode_c64(&data) : 0;
    tinfo->lex_filepos  = Math_decode_c64(&data);

    /* Don't allow buffer overruns. */
    if (data > self->limit) {
        THROW("Buffer overrun in lexicon index for %o",
            InStream_Get_Filename(self->ix_in));
    }
}

void
LexIndex_seek(LexIndex *self, Obj *target)
{
    FieldType   *type   = self->field_type;
    i32_t        lo     = 0;
    i32_t        hi     = self->size - 1;
    i32_t        result = -100;

    if (target == NULL || self->size == 0) { 
        self->tick = 0;
        return;
    }
    else {
        if ( !OBJ_IS_A(target, CHARBUF)) {
            THROW("Target is a %o, and not comparable to a %o",
                Obj_Get_Class_Name(target), CHARBUF.name);
        }
        /* TODO: 
        Obj *first_obj = VA_Fetch(terms, 0);
        if ( !Obj_Is_A(target, Obj_Get_VTable(first_obj)) ) {
            THROW("Target is a %o, and not comparable to a %o",
                Obj_Get_Class_Name(target), Obj_Get_Class_Name(first_obj));
        }
        */
    }

    /* Divide and conquer. */
    while (hi >= lo) {
        const i32_t mid = lo + ((hi - lo) / 2);
        const i64_t offset 
            = (i64_t)Math_decode_bigend_u64(self->offsets + mid);
        char *data = self->data + offset;
        size_t size = Math_decode_c32(&data);
        i64_t comparison;

        ViewCB_Assign_Str(self->term, data, size);
        comparison = FType_Compare_Values(type, target, (Obj*)self->term);
        if (comparison < 0) {
            hi = mid - 1;
        }
        else if (comparison > 0) {
            lo = mid + 1;
        }
        else {
            result = mid;
            break;
        }
    }

    /* Record the index of the entry we've seeked to, then read entry. */
    self->tick = hi == -1   ? 0  /* indicating that target lt first entry */
           : result == -100 ? hi /* if result is still -100, it wasn't set */
           : result;
    S_read_entry(self);
}

/* Copyright 2006-2009 Marvin Humphrey
 *
 * This program is free software; you can redistribute it and/or modify
 * under the same terms as Perl itself.
 */

