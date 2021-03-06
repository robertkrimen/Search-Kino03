use Search::Kino03::KinoSearch;

1;

__END__

__XS__

MODULE = Search::Kino03::KinoSearch    PACKAGE = Search::Kino03::KinoSearch::Store::InStream

SV*
new(class_name, file_des)
    kino_ClassNameBuf class_name;
    kino_FileDes *file_des;
CODE:
{
    kino_VTable *vtable 
        = kino_VTable_singleton((kino_CharBuf*)&class_name, NULL);
    kino_InStream *self = (kino_InStream*)Kino_VTable_Make_Obj(vtable);
    kino_InStream_init(self, file_des);
    KOBJ_TO_SV_NOINC(self, RETVAL);
}
OUTPUT: RETVAL

kino_InStream*
reopen(self, filename, offset, len)
    kino_InStream *self;
    kino_CharBuf filename;
    chy_u64_t offset;
    chy_u64_t len;
CODE:
    RETVAL = Kino_InStream_Reopen(self, &filename, offset, len);
OUTPUT: RETVAL

void
read_bytes(self, buffer_sv, len)
    kino_InStream *self;
    SV *buffer_sv;
    size_t len;
PPCODE:
{
    char *ptr;
    SvUPGRADE(buffer_sv, SVt_PV);
    ptr = SvGROW(buffer_sv, len + 1);
    Kino_InStream_Read_Bytes(self, ptr, len);
    if (!SvPOK(buffer_sv)) SvCUR_set(buffer_sv, 0);
    SvPOK_on(buffer_sv);
    if (SvCUR(buffer_sv) < len) {
        SvCUR_set(buffer_sv, len);
        *(SvEND(buffer_sv)) = '\0';
    }
}

void
read_byteso(self, buffer_sv, start, len)
    kino_InStream *self;
    SV *buffer_sv;
    size_t start;
    size_t len;
PPCODE:
{
    size_t total_len = start + len;
    char *ptr;
    SvUPGRADE(buffer_sv, SVt_PV);
    if (!SvPOK(buffer_sv)) SvCUR_set(buffer_sv, 0);
    ptr = SvGROW(buffer_sv, total_len + 1);
    Kino_InStream_Read_BytesO(self, ptr, start, len);
    SvPOK_on(buffer_sv);
    if (SvCUR(buffer_sv) < total_len) {
        SvCUR_set(buffer_sv, total_len);
        *(SvEND(buffer_sv)) = '\0';
    }
}

SV*
read_string(self)
    kino_InStream *self;
CODE:
{
    char *ptr;
    size_t len = Kino_InStream_Read_C32(self);
    RETVAL = newSV(len + 1);
    SvCUR_set(RETVAL, len);
    SvPOK_on(RETVAL);
    SvUTF8_on(RETVAL); /* Trust source.  Reconsider if API goes public. */
    *SvEND(RETVAL) = '\0';
    ptr = SvPVX(RETVAL);
    Kino_InStream_Read_Bytes(self, ptr, len);
}
OUTPUT: RETVAL

int
read_raw_c64(self, buffer_sv)
    kino_InStream *self;
    SV *buffer_sv;
CODE:
{
    char *ptr;
    SvUPGRADE(buffer_sv, SVt_PV);
    ptr = SvGROW(buffer_sv, 10 + 1);
    RETVAL = Kino_InStream_Read_Raw_C64(self, ptr);
    SvPOK_on(buffer_sv);
    SvCUR_set(buffer_sv, RETVAL);
}
OUTPUT: RETVAL

__AUTO_XS__

{   "Search::Kino03::KinoSearch::Store::InStream" => {
        bind_methods => [
            qw( Seek
                Tell
                Length
                Close
                Read_I8
                Read_I32
                Read_I64
                Read_U8
                Read_U32
                Read_U64
                Read_C32
                Read_C64
                Read_Float )
        ],
        make_getters => [qw( offset file_des )],
    }
}

__COPYRIGHT__

Copyright 2005-2009 Marvin Humphrey

This program is free software; you can redistribute it and/or modify
under the same terms as Perl itself.

