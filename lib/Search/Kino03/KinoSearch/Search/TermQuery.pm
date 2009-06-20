use Search::Kino03::KinoSearch;

1;

__END__

__XS__

MODULE = Search::Kino03::KinoSearch   PACKAGE = Search::Kino03::KinoSearch::Search::TermQuery

SV*
get_term(self)
    kino_TermQuery *self;
CODE:
{
    kino_Obj *term = Kino_TermQuery_Get_Term(self);
    if (KINO_OBJ_IS_A(term, KINO_CHARBUF)) {
         RETVAL = XSBind_cb_to_sv((kino_CharBuf*)term);
    }
    else {
        RETVAL = Kino_Obj_To_Host(term);
    }
}
OUTPUT: RETVAL

__AUTO_XS__

my $synopsis = <<'END_SYNOPSIS';
    my $term_query = Search::Kino03::KinoSearch::Search::TermQuery->new(
        field => 'content',
        term  => 'foo', 
    );
    my $hits = $searcher->hits( query => $term_query );
END_SYNOPSIS

my $constructor = <<'END_CONSTRUCTOR';
    my $term_query = Search::Kino03::KinoSearch::Search::TermQuery->new(
        field => 'content',    # required
        term  => 'foo',        # required
    );
END_CONSTRUCTOR

{   "Search::Kino03::KinoSearch::Search::TermQuery" => {
        bind_methods      => [qw( Get_Field )],
        make_constructors => ["new"],
        make_pod          => {
            synopsis    => $synopsis,
            constructor => { sample => $constructor },
        },
    },
    "Search::Kino03::KinoSearch::Search::TermCompiler" =>
        { make_constructors => ["do_new"], },
}

__COPYRIGHT__

Copyright 2005-2009 Marvin Humphrey

This program is free software; you can redistribute it and/or modify
under the same terms as Perl itself.

