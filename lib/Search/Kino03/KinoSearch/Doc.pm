use Search::Kino03::KinoSearch;

1;

__END__

__XS__

MODULE = Search::Kino03::KinoSearch     PACKAGE = Search::Kino03::KinoSearch::Doc

SV*
get_fields(self, ...)
    kino_Doc *self;
CODE:
    CHY_UNUSED_VAR(items);
    RETVAL = newRV_inc( (SV*)Kino_Doc_Get_Fields(self) );
OUTPUT: RETVAL

__AUTO_XS__

my $synopsis = <<'END_SYNOPSIS';
    my $doc = Search::Kino03::KinoSearch::Doc->new(
        fields => { foo => 'foo foo', bar => 'bar bar' },
    );
    $doc->{foo} = 'new value for field "foo"';
    $indexer->add_doc($doc);
END_SYNOPSIS

my $constructor = <<'END_CONSTRUCTOR';
    my $doc = Search::Kino03::KinoSearch::Doc->new(
        fields => { foo => 'foo foo', bar => 'bar bar' },
    );
END_CONSTRUCTOR

{   "Search::Kino03::KinoSearch::Doc" => {
        make_constructors => ['new'],
        bind_methods => [ qw( Set_Doc_ID Get_Doc_ID Set_Fields ) ],
        make_pod => {
            methods     => [qw( get_fields )],
            synopsis    => $synopsis,
            constructor => { sample => $constructor },
        }
    },
}

__COPYRIGHT__

Copyright 2005-2009 Marvin Humphrey

This program is free software; you can redistribute it and/or modify
under the same terms as Perl itself.

