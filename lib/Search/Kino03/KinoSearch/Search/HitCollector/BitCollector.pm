use Search::Kino03::KinoSearch;

1;

__END__

__AUTO_XS__

my $synopsis = <<'END_SYNOPSIS';
    my $bit_vec = Search::Kino03::KinoSearch::Util::BitVector->new(
        capacity => $searcher->doc_max + 1,
    );
    my $bit_collector = Search::Kino03::KinoSearch::Search::HitCollector::BitCollector->new(
        bit_vector => $bit_vec, 
    );
    $searcher->collect(
        collector => $bit_collector,
        query     => $query,
    );
END_SYNOPSIS

my $constructor = <<'END_CONSTRUCTOR';
    my $bit_collector = Search::Kino03::KinoSearch::Search::HitCollector::BitCollector->new(
        bit_vector => $bit_vec,    # required
    );
END_CONSTRUCTOR

{   "Search::Kino03::KinoSearch::Search::HitCollector::BitCollector" => {
        make_constructors => ["new"],
        make_pod          => {
            synopsis    => $synopsis,
            constructor => { sample => $constructor },
            methods     => [qw( collect )],
        },
    },
}

__COPYRIGHT__

Copyright 2005-2009 Marvin Humphrey

This program is free software; you can redistribute it and/or modify
under the same terms as Perl itself.

