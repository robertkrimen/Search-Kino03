#include "Search/Kino03/KinoSearch/Util/ToolSet.h"

#include "Search/Kino03/KinoSearch/FieldType/BlobType.h"

BlobType*
BlobType_new()
{
    BlobType *self = (BlobType*)VTable_Make_Obj(&BLOBTYPE);
    return BlobType_init(self);
}

BlobType*
BlobType_init(BlobType *self)
{
    FType_init((FieldType*)self);
    self->stored     = true;
    return self;
}

bool_t
BlobType_binary(BlobType *self)
{
    UNUSED_VAR(self);
    return true;
}

void
BlobType_set_sortable(BlobType *self, bool_t sortable)
{
    UNUSED_VAR(self);
    if (sortable) { THROW("BlobType fields can't be sortable"); }
}

bool_t
BlobType_equals(BlobType *self, Obj *other)
{
    BlobType *evil_twin = (BlobType*)other;
    if (evil_twin == self) return true;
    if (!OBJ_IS_A(evil_twin, BLOBTYPE)) return false;
    return FType_equals((FieldType*)self, other);
}

Hash*
BlobType_dump_for_schema(BlobType *self) 
{
    Hash *dump = Hash_new(0);
    Hash_Store_Str(dump, "type", 4, (Obj*)CB_newf("blob"));

    /* Store attributes that override the defaults -- even if they're
     * meaningless. */
    if (self->boost != 1.0) {
        Hash_Store_Str(dump, "boost", 5, (Obj*)CB_newf("%f64", self->boost));
    }
    if (self->indexed) {
        Hash_Store_Str(dump, "indexed", 7, (Obj*)CB_newf("1"));
    }
    if (!self->stored) {
        Hash_Store_Str(dump, "stored", 6, (Obj*)CB_newf("0"));
    }

    return dump;
}

Hash*
BlobType_dump(BlobType *self)
{
    Hash *dump = BlobType_Dump_For_Schema(self);
    Hash_Store_Str(dump, "_class", 6, 
        (Obj*)CB_Clone(Obj_Get_Class_Name(self)));
    return dump;
}

BlobType*
BlobType_load(BlobType *self, Obj *dump)
{
    Hash *source = (Hash*)ASSERT_IS_A(dump, HASH);
    CharBuf *class_name = (CharBuf*)Hash_Fetch_Str(source, "_class", 6);
    VTable *vtable = (class_name != NULL && OBJ_IS_A(class_name, CHARBUF)) 
                   ? VTable_singleton(class_name, NULL)
                   : (VTable*)&BLOBTYPE;
    BlobType *loaded     = (BlobType*)VTable_Make_Obj(vtable);
    Obj *boost_dump      = Hash_Fetch_Str(source, "boost", 5);
    Obj *indexed_dump    = Hash_Fetch_Str(source, "indexed", 7);
    Obj *stored_dump     = Hash_Fetch_Str(source, "stored", 6);

    BlobType_init(loaded);
    if (boost_dump)   { self->boost   = (float)Obj_To_F64(boost_dump);    }
    if (indexed_dump) { self->indexed = (bool_t)Obj_To_I64(indexed_dump); }
    if (stored_dump)  { self->stored  = (bool_t)Obj_To_I64(stored_dump);  }

    return loaded;
}

/* Copyright 2007-2009 Marvin Humphrey
 *
 * This program is free software; you can redistribute it and/or modify
 * under the same terms as Perl itself.
 */

