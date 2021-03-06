use Search::Kino03::KinoSearch;

1;

__END__

__XS__

MODULE = Search::Kino03::KinoSearch   PACKAGE = Search::Kino03::KinoSearch::Util::Float64

SV*
new(class_name, value)
    kino_ClassNameBuf class_name;
    double value;
CODE:
{
    kino_VTable *vtable 
        = kino_VTable_singleton((kino_CharBuf*)&class_name, NULL);
    kino_Float64 *self = (kino_Float64*)Kino_VTable_Make_Obj(vtable);
    kino_Float64_init(self, value);
    KOBJ_TO_SV_NOINC(self, RETVAL);
}
OUTPUT: RETVAL

__AUTO_XS__

{   "Search::Kino03::KinoSearch::Util::Float64" => {
        bind_methods => [qw( Set_Value Get_Value )],
    }
}

__COPYRIGHT__

Copyright 2005-2009 Marvin Humphrey

This program is free software; you can redistribute it and/or modify
under the same terms as Perl itself.

