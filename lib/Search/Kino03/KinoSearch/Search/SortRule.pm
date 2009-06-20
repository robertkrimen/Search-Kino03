use Search::Kino03::KinoSearch;

1;

__END__

__XS__

MODULE = Search::Kino03::KinoSearch   PACKAGE = Search::Kino03::KinoSearch::Search::SortRule

chy_i32_t
FIELD()
CODE:
    RETVAL = kino_SortRule_FIELD;
OUTPUT: RETVAL

chy_i32_t
SCORE()
CODE:
    RETVAL = kino_SortRule_SCORE;
OUTPUT: RETVAL

chy_i32_t
DOC_ID()
CODE:
    RETVAL = kino_SortRule_DOC_ID;
OUTPUT: RETVAL

__AUTO_XS__

my $synopsis = <<'END_SYNOPSIS';
    my $sort_spec = Search::Kino03::KinoSearch::Search::SortSpec->new(
        rules => [
            Search::Kino03::KinoSearch::Search::SortRule->new( field => 'date' ),
            Search::Kino03::KinoSearch::Search::SortRule->new( type  => 'doc_id' ),
        ],
    );
END_SYNOPSIS

my $constructor = <<'END_CONSTRUCTOR';
    my $by_title   = Search::Kino03::KinoSearch::Search::SortRule->new( field => 'title' );
    my $by_score   = Search::Kino03::KinoSearch::Search::SortRule->new( type  => 'score' );
    my $by_doc_id  = Search::Kino03::KinoSearch::Search::SortRule->new( type  => 'doc_id' );
    my $reverse_date = Search::Kino03::KinoSearch::Search::SortRule->new(
        field   => 'date',
        reverse => 1,
    );
END_CONSTRUCTOR

{   "Search::Kino03::KinoSearch::Search::SortRule" => {
        make_constructors => ["_new"],
        bind_methods      => [qw( Get_Field Get_Reverse )],
        make_pod          => {
            synopsis    => $synopsis,
            constructor => { sample => $constructor },
            methods     => [qw( get_field get_reverse )],
        },
    },
}

__COPYRIGHT__

Copyright 2005-2009 Marvin Humphrey

This program is free software; you can redistribute it and/or modify
under the same terms as Perl itself.

