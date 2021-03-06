#include "xs/XSBind.h"
#include "Search/Kino03/KinoSearch/Doc.h"
#include "Search/Kino03/KinoSearch/Store/InStream.h"
#include "Search/Kino03/KinoSearch/Store/OutStream.h"
#include "Search/Kino03/KinoSearch/Util/Host.h"
#include "Search/Kino03/KinoSearch/Util/MemManager.h"

kino_Doc*
kino_Doc_init(kino_Doc *self, void *fields, chy_i32_t doc_id)
{
    /* Assign. */
    if (fields) {
        if (SvTYPE((SV*)fields) != SVt_PVHV) THROW("Not a hash");
        self->fields = SvREFCNT_inc((SV*)fields);
    }
    else {
        self->fields = newHV();
    }
    self->doc_id = doc_id;

    return self;
}

void
kino_Doc_set_fields(kino_Doc *self, void *fields)
{
    if (self->fields) SvREFCNT_dec((SV*)self->fields);
    self->fields = SvREFCNT_inc((SV*)fields);
}

chy_u32_t
kino_Doc_get_size(kino_Doc *self)
{
    return self->fields ? HvKEYS((HV*)self->fields) : 0;
}

void
kino_Doc_store(kino_Doc *self, const kino_CharBuf *field, kino_Obj *value)
{
    char   *key      = (char*)Kino_CB_Get_Ptr8(field);
    size_t  key_size = Kino_CB_Get_Size(field);
    SV *val_sv = value == NULL
               ? newSV(0)
               : KINO_OBJ_IS_A(value, KINO_CHARBUF) 
               ? XSBind_cb_to_sv((kino_CharBuf*)value)
               : Kino_Obj_To_Host(value);
    hv_store((HV*)self->fields, key, key_size, val_sv, 0);
}

void
kino_Doc_serialize(kino_Doc *self, kino_OutStream *outstream)
{
    Kino_OutStream_Write_C32(outstream, self->doc_id);
    kino_Host_callback(self, "serialize_fields", 1, 
        KINO_ARG_OBJ("outstream", outstream));
}

kino_Doc*
kino_Doc_deserialize(kino_Doc *self, kino_InStream *instream)
{
    chy_i32_t doc_id     = (chy_i32_t)Kino_InStream_Read_C32(instream);

    self = self ? self : (kino_Doc*)Kino_VTable_Make_Obj(&KINO_DOC);
    kino_Doc_init(self, NULL, doc_id);
    kino_Host_callback(self, "deserialize_fields", 1, 
        KINO_ARG_OBJ("instream", instream));
    
    return self;
}

kino_Obj*
kino_Doc_extract(kino_Doc *self, kino_CharBuf *field, 
                 kino_ViewCharBuf *target) 
{
    kino_Obj *retval = NULL;
    SV **sv_ptr = hv_fetch(self->fields, field->ptr, 
        Kino_CB_Get_Size(field), 0);

    if (sv_ptr && XSBind_sv_defined(*sv_ptr)) {
        SV *const sv = *sv_ptr;
        if (sv_isobject(sv) && sv_derived_from(sv, KINO_OBJ.name->ptr)) {
            IV tmp = SvIV( SvRV(sv) );
            retval = INT2PTR(kino_Obj*, tmp);
        }
        else {
            STRLEN size;
            char *ptr = SvPVutf8(sv, size);
            Kino_ViewCB_Assign_Str(target, ptr, size);
            retval = (kino_Obj*)target;
        }
    }

    return retval;
}
    
void*
kino_Doc_to_host(kino_Doc *self)
{
    SV *perl_obj = kino_Obj_to_host((kino_Obj*)self); /* SUPER */
    HV *stash;
    kino_CharBuf *class_name = Kino_Obj_Get_Class_Name(self);
    chy_u8_t *ptr = Kino_CB_Get_Ptr8(class_name);
    size_t size = Kino_CB_Get_Size(class_name);

    /* This code is informed by the following snippet from Perl_sv_bless, from
     * sv.c:
     *
     *     if (Gv_AMG(stash))
     *         SvAMAGIC_on(sv);
     *     else
     *         (void)SvAMAGIC_off(sv);
     *
     * Gv_AMupdate is undocumented.  It is extracted from the Gv_AMG macro,
     * also undocumented, defined in sv.h:
     *
     *     #define Gv_AMG(stash)  (PL_amagic_generation && Gv_AMupdate(stash))
     * 
     * The purpose of the code is to turn on overloading for the class in
     * question.  It seems that as soon as overloading is on for any class,
     * anywhere, that PL_amagic_generation goes positive and stays positive,
     * so that Gv_AMupdate gets called with every bless() invocation.  Since
     * we need overloading for Doc and all its subclasses, we skip the check
     * and just update every time.
     */
    stash = gv_stashpvn((char*)ptr, size, true);
    Gv_AMupdate(stash);
    SvAMAGIC_on(perl_obj);

    return perl_obj;
}

kino_Hash*
kino_Doc_dump(kino_Doc *self)
{
    kino_Hash *dump = kino_Hash_new(0);
    Kino_Hash_Store_Str(dump, "_class", 6, 
        (kino_Obj*)Kino_CB_Clone(Kino_Obj_Get_Class_Name(self)));
    Kino_Hash_Store_Str(dump, "doc_id", 7, 
        (kino_Obj*)kino_CB_newf("%i32", self->doc_id));
    kino_Hash_store_str(dump, "fields", 6, XSBind_perl_to_kino(self->fields));
    return dump;
}

kino_Doc*
kino_Doc_load(kino_Doc *self, kino_Obj *dump)
{
    kino_Hash *source = (kino_Hash*)KINO_ASSERT_IS_A(dump, KINO_HASH);
    kino_CharBuf *class_name = (kino_CharBuf*)KINO_ASSERT_IS_A(
        Kino_Hash_Fetch_Str(source, "_class", 6), KINO_CHARBUF);
    kino_VTable *vtable = kino_VTable_singleton(class_name, NULL);
    kino_Doc *loaded = (kino_Doc*)Kino_VTable_Make_Obj(vtable);
    kino_Obj *doc_id = KINO_ASSERT_IS_A(
        Kino_Hash_Fetch_Str(source, "doc_id", 7), KINO_OBJ);
    kino_Hash *fields = (kino_Hash*)KINO_ASSERT_IS_A(
        Kino_Hash_Fetch_Str(source, "fields", 6), KINO_HASH);
    SV *fields_sv = XSBind_kobj_to_pobj((kino_Obj*)fields);
    CHY_UNUSED_VAR(self);

    loaded->doc_id = (chy_i32_t)Kino_Obj_To_I64(doc_id);
    loaded->fields  = SvREFCNT_inc(SvRV(fields_sv));
    SvREFCNT_dec(fields_sv);

    return loaded;
}

chy_bool_t
kino_Doc_equals(kino_Doc *self, kino_Obj *other)
{
    kino_Doc *evil_twin = (kino_Doc*)other;
    HV *my_fields;
    HV *other_fields;
    I32 num_fields;

    if (evil_twin == self)                    { return true;  }
    if (!KINO_OBJ_IS_A(evil_twin, KINO_DOC))  { return false; }
    if (!self->doc_id == evil_twin->doc_id)   { return false; }
    if (!!self->fields ^ !!evil_twin->fields) { return false; }

    /* Verify fields.  Don't allow any deep data structures. */
    my_fields    = (HV*)self->fields;
    other_fields = (HV*)evil_twin->fields;
    if (HvKEYS(my_fields) != HvKEYS(other_fields)) { return false; }
    num_fields = hv_iterinit(my_fields);
    while(num_fields--) {
        HE *my_entry = hv_iternext(my_fields);
        SV *my_val_sv = HeVAL(my_entry);
        STRLEN key_len = HeKLEN(my_entry);
        char *key = HeKEY(my_entry);
        SV **const other_val = hv_fetch(other_fields, key, key_len, 0);
        if (!other_val) { return false; }
        if (!sv_eq(my_val_sv, *other_val)) { return false; }
    }

    return true;
}

void
kino_Doc_destroy(kino_Doc *self)
{
    if (self->fields) SvREFCNT_dec((SV*)self->fields);
    KINO_FREE_OBJ(self);
}

/* Copyright 2007-2009 Marvin Humphrey
 *
 * This program is free software; you can redistribute it and/or modify
 * under the same terms as Perl itself.
 */

