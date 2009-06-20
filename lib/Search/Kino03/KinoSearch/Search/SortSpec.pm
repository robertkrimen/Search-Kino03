use Search::Kino03::KinoSearch;

1;

__END__


__AUTO_XS__


my $synopsis = <<'END_SYNOPSIS';
    my $sort_spec = Search::Kino03::KinoSearch::Search::SortSpec->new(
        rules => [
            Search::Kino03::KinoSearch::Search::SortRule->new( field => 'date' ),
            Search::Kino03::KinoSearch::Search::SortRule->new( type  => 'doc_id' ),
        ],
    );
    my $hits = $searcher->hits(
        query     => $query,
        sort_spec => $sort_spec,
    );
END_SYNOPSIS

my $constructor = <<'END_CONSTRUCTOR';
    my $sort_spec = Search::Kino03::KinoSearch::Search::SortSpec->new( rules => \@rules );
END_CONSTRUCTOR

{   "Search::Kino03::KinoSearch::Search::SortSpec" => {
        bind_methods      => [qw( Get_Rules )],
        make_constructors => ["new"],
        make_pod          => {
            synopsis    => $synopsis,
            constructor => { sample => $constructor },
        },
    },
}

__COPYRIGHT__

Copyright 2005-2009 Marvin Humphrey

This program is free software; you can redistribute it and/or modify
under the same terms as Perl itself.

