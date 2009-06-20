use Search::Kino03::KinoSearch;

1;

__END__

__AUTO_XS__

{   "Search::Kino03::KinoSearch::Index::SortCache" => {
        make_constructors => ["new"],
        bind_methods      => [qw( Ordinal Find )],
    }
}

__XS__

MODULE = Search::Kino03::KinoSearch   PACKAGE = Search::Kino03::KinoSearch::Index::SortCache

SV*
value(self, ...)
    kino_SortCache *self;
CODE:
{
    HV *const args_hash = XSBind_build_args_hash( &(ST(0)), 1, items,
        "Search::Kino03::KinoSearch::Index::SortCache::value_PARAMS");
    chy_i32_t ord = XSBind_extract_iv(args_hash, SNL("ord"));
    kino_ZombieCharBuf temp = KINO_ZCB_BLANK;
    kino_ViewCharBuf *value = Kino_SortCache_Value(self, ord,
        (kino_ViewCharBuf*)&temp);
    RETVAL = XSBind_cb_to_sv((kino_CharBuf*)value);
}
OUTPUT: RETVAL

__COPYRIGHT__

Copyright 2005-2009 Marvin Humphrey

This program is free software; you can redistribute it and/or modify
under the same terms as Perl itself.

