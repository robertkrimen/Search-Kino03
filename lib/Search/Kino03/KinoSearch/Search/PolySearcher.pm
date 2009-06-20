use Search::Kino03::KinoSearch;

1;

__END__

__AUTO_XS__

my $synopsis = <<'END_SYNOPSIS';
    my $schema = MySchema->new;
    for my $server_name (@server_names) {
        push @searchers, Search::Kino03::KSx::Remote::SearchClient->new(
            peer_address => "$server_name:$port",
            password     => $pass,
            schema       => $schema,
        );
    }
    my $poly_searcher = Search::Kino03::KinoSearch::Search::PolySearcher->new(
        schema      => $schema,
        searchables => \@searchers,
    );
    my $hits = $poly_searcher->hits( query => $query );
END_SYNOPSIS

my $constructor = <<'END_CONSTRUCTOR';
    my $poly_searcher = Search::Kino03::KinoSearch::Search::PolySearcher->new(
        schema      => $schema,
        searchables => \@searchers,
    );
END_CONSTRUCTOR

{   "Search::Kino03::KinoSearch::Search::PolySearcher" => {
        make_getters      => [qw( searchables starts )],
        make_constructors => ["new"],
        make_pod          => {
            synopsis    => $synopsis,
            constructor => { sample => $constructor },
        }
    }
}

__COPYRIGHT__

Copyright 2005-2009 Marvin Humphrey

This program is free software; you can redistribute it and/or modify
under the same terms as Perl itself.

