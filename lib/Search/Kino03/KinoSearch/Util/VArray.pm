use Search::Kino03::KinoSearch;

1;

__END__

__XS__

MODULE = Search::Kino03::KinoSearch   PACKAGE = Search::Kino03::KinoSearch::Util::VArray

SV*
shallow_copy(self)
    kino_VArray *self;
CODE:
    KOBJ_TO_SV_NOINC(Kino_VA_Shallow_Copy(self), RETVAL);
OUTPUT: RETVAL

SV*
_deserialize(either_sv, instream)
    SV *either_sv;
    kino_InStream *instream;
CODE:
    CHY_UNUSED_VAR(either_sv);
    KOBJ_TO_SV_NOINC(kino_VA_deserialize(NULL, instream), RETVAL);
OUTPUT: RETVAL

SV*
_clone(self)
    kino_VArray *self;
CODE:
    KOBJ_TO_SV_NOINC(Kino_VA_Clone(self), RETVAL);
OUTPUT: RETVAL

SV*
shift(self)
    kino_VArray *self;
CODE:
    KOBJ_TO_SV_NOINC(Kino_VA_Shift(self), RETVAL);
OUTPUT: RETVAL

SV*
pop(self)
    kino_VArray *self;
CODE:
    KOBJ_TO_SV_NOINC(Kino_VA_Pop(self), RETVAL);
OUTPUT: RETVAL

SV*
delete(self, tick)
    kino_VArray *self;
    chy_u32_t    tick;
CODE:
    KOBJ_TO_SV_NOINC(Kino_VA_Delete(self, tick), RETVAL);
OUTPUT: RETVAL

SV*
fetch(self, tick)
    kino_VArray *self;
    chy_u32_t    tick;
CODE:
    KOBJ_TO_SV(Kino_VA_Fetch(self, tick), RETVAL);
OUTPUT: RETVAL

__AUTO_XS__

{   "Search::Kino03::KinoSearch::Util::VArray" => {
        bind_positional => [qw( Store )],
        bind_methods    => [
            qw(
                Push
                Push_VArray
                Unshift
                Splice
                Resize
                Get_Size
                )
        ],
        make_constructors => ["new"],
    }
}

__COPYRIGHT__

Copyright 2005-2009 Marvin Humphrey

This program is free software; you can redistribute it and/or modify
under the same terms as Perl itself.

