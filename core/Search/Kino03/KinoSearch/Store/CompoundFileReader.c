#include "Search/Kino03/KinoSearch/Util/ToolSet.h"

#include "Search/Kino03/KinoSearch/Store/CompoundFileReader.h"
#include "Search/Kino03/KinoSearch/Index/Segment.h"
#include "Search/Kino03/KinoSearch/Store/CompoundFileWriter.h"
#include "Search/Kino03/KinoSearch/Store/FSFolder.h"
#include "Search/Kino03/KinoSearch/Store/InStream.h"
#include "Search/Kino03/KinoSearch/Util/Json.h"
#include "Search/Kino03/KinoSearch/Util/StringHelper.h"

CompoundFileReader*
CFReader_new(FSFolder *folder, const CharBuf *seg_name)
{
    CompoundFileReader *self 
        = (CompoundFileReader*)VTable_Make_Obj(&COMPOUNDFILEREADER);
    return CFReader_init(self, folder, seg_name);
}

CompoundFileReader*
CFReader_init(CompoundFileReader *self, FSFolder *folder, 
              const CharBuf *seg_name)
{
    CharBuf *cf_file     = CB_newf("%o%scf.dat", seg_name, DIR_SEP);
    CharBuf *cfmeta_file = CB_newf("%o%scfmeta.json", seg_name, DIR_SEP);
    Hash *metadata = (Hash*)Json_slurp_json((Folder*)folder, cfmeta_file);

    /* Open an instream which we'll clone over and over. */
    self->instream = FSFolder_Open_In(folder, cf_file);

    /* Recover gracefully if a file can't be found. */
    if (!metadata  || !self->instream) {
        DECREF(cf_file);
        DECREF(cfmeta_file);
        DECREF(self);
        return NULL;
    }

    /* Check format. */
    ASSERT_IS_A(metadata, HASH);
    {
        Obj *format = Hash_Fetch_Str(metadata, "format", 6);
        if (!format) { THROW("Missing 'format'"); }
        else if (Obj_To_I64(format) > CFWriter_current_file_format) {
            THROW("Unsupported compound file format: %i64", 
                Obj_To_I64(format));
        }
    }

    /* Assign/derive. */
    self->seg_name = CB_Clone(seg_name);
    self->entries = (Hash*)Hash_Fetch_Str(metadata, "files", 5);
    ASSERT_IS_A(self->entries, HASH);
    INCREF(self->entries);

    DECREF(metadata);
    DECREF(cf_file);
    DECREF(cfmeta_file);

    return self;
}

void
CFReader_destroy(CompoundFileReader *self)
{
    DECREF(self->instream);
    DECREF(self->entries);
    DECREF(self->seg_name);
    FREE_OBJ(self);
}

i32_t
CFReader_get_size(CompoundFileReader *self)
{
    return Hash_Get_Size(self->entries);
}

bool_t
CFReader_delete_virtual(CompoundFileReader *self, const CharBuf *filepath)
{
    Hash *entry = (Hash*)Hash_Fetch(self->entries, filepath);
    if (entry == NULL) { 
        THROW("File '%o' doesn't exist", filepath); 
    }
    else { 
        Hash_Delete(self->entries, filepath); 
        DECREF(entry); 
    }
    return true;
}

static void
S_retrieve_offset_and_len(CompoundFileReader *self, const CharBuf *filepath,
                          i64_t *offset, i64_t *len)
{
    Hash *entry = (Hash*)Hash_Fetch(self->entries, filepath);
    if (entry == NULL)
        THROW("Couldn't find entry for '%o'", filepath);
    
    *offset = Obj_To_I64(Hash_Fetch_Str(entry, "offset", 6));
    *len    = Obj_To_I64(Hash_Fetch_Str(entry, "length", 6));
}

InStream*
CFReader_open_in(CompoundFileReader *self, const CharBuf *filepath)
{
    i64_t len;
    i64_t offset;

    S_retrieve_offset_and_len(self, filepath, &offset, &len);

    return InStream_Reopen(self->instream, filepath, offset, len);
}

bool_t
CFReader_exists(CompoundFileReader *self, const CharBuf *filepath)
{
    return Hash_Fetch(self->entries, filepath) ? true : false;
}

VArray*
CFReader_list(CompoundFileReader *self) {
    return Hash_keys(self->entries);
}

void
CFReader_close(CompoundFileReader *self)
{
    InStream_Close(self->instream);
}

/* Copyright 2006-2009 Marvin Humphrey
 *
 * This program is free software; you can redistribute it and/or modify
 * under the same terms as Perl itself.
 */

