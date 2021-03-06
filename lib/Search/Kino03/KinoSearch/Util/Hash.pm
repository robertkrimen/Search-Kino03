use Search::Kino03::KinoSearch;

1;

__END__

__XS__

MODULE =  KinoSearch    PACKAGE = Search::Kino03::KinoSearch::Util::Hash

SV*
_deserialize(either_sv, instream)
    SV *either_sv;
    kino_InStream *instream;
CODE:
    CHY_UNUSED_VAR(either_sv);
    KOBJ_TO_SV_NOINC(kino_Hash_deserialize(NULL, instream), RETVAL);
OUTPUT: RETVAL

void
iter_next(self)
    kino_Hash *self;
PPCODE:
{
    kino_CharBuf *key;
    kino_Obj     *val;

    if (Kino_Hash_Iter_Next(self, &key, &val)) {
        SV *key_sv = Kino_Obj_To_Host(key);
        SV *val_sv = Kino_Obj_To_Host(val);

        XPUSHs(sv_2mortal( key_sv ));
        XPUSHs(sv_2mortal( val_sv ));
        XSRETURN(2);
    }
    else {
        XSRETURN_EMPTY;
    }
}

SV*
fetch(self, key)
    kino_Hash *self;
    kino_CharBuf key;
CODE:
    KOBJ_TO_SV( Kino_Hash_Fetch(self, &key), RETVAL );
OUTPUT: RETVAL

SV*
delete(self, key)
    kino_Hash *self;
    kino_CharBuf key;
CODE:
    KOBJ_TO_SV_NOINC( Kino_Hash_Delete(self, &key), RETVAL );
OUTPUT: RETVAL

SV*
keys(self)
    kino_Hash *self;
CODE:
    KOBJ_TO_SV_NOINC( Kino_Hash_Keys(self), RETVAL );
OUTPUT: RETVAL

SV*
values(self)
    kino_Hash *self;
CODE:
    KOBJ_TO_SV_NOINC( Kino_Hash_Values(self), RETVAL );
OUTPUT: RETVAL

__AUTO_XS__

{   "Search::Kino03::KinoSearch::Util::Hash" => {
        bind_positional   => [qw( Store )],
        bind_methods      => [qw( Find_Key Clear Iter_Init Get_Size )],
        make_constructors => ["new"],
    }
}

__COPYRIGHT__

Copyright 2005-2009 Marvin Humphrey

This program is free software; you can redistribute it and/or modify
under the same terms as Perl itself.

