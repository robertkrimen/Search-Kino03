use Search::Kino03::KinoSearch;

1;

__END__

__AUTO_XS__

my $synopsis = <<'END_SYNOPSIS';
    my $foo_or_bar_query = Search::Kino03::KinoSearch::Search::ORQuery->new(
        children => [ $foo_query, $bar_query ],
    );
    my $hits = $searcher->hits( query => $foo_or_bar_query );
    ...
END_SYNOPSIS

my $constructor = <<'END_CONSTRUCTOR';
    my $foo_or_bar_query = Search::Kino03::KinoSearch::Search::ORQuery->new(
        children => [ $foo_query, $bar_query ],
    );
END_CONSTRUCTOR

{   "Search::Kino03::KinoSearch::Search::ORQuery" => {
        make_constructors => ["new"],
        make_pod          => {
            methods     => [qw( add_child )],
            synopsis    => $synopsis,
            constructor => { sample => $constructor, }
        },
    },
}

__COPYRIGHT__

Copyright 2005-2009 Marvin Humphrey

This program is free software; you can redistribute it and/or modify
under the same terms as Perl itself.

