use Search::Kino03::KinoSearch;

1;

__END__

__AUTO_XS__

my $synopsis = <<'END_SYNOPSIS';
    my $foo_and_maybe_bar = Search::Kino03::KinoSearch::Search::RequiredOptionalQuery->new(
        required_query => $foo_query,
        optional_query => $bar_query,
    );
    my $hits = $searcher->hits( query => $foo_and_maybe_bar );
    ...
END_SYNOPSIS

my $constructor = <<'END_CONSTRUCTOR';
    my $reqopt_query = Search::Kino03::KinoSearch::Search::RequiredOptionalQuery->new(
        required_query => $foo_query,    # required
        optional_query => $bar_query,    # required
    );
END_CONSTRUCTOR

{   "Search::Kino03::KinoSearch::Search::RequiredOptionalQuery" => {
        bind_methods => [
            qw( Get_Required_Query Set_Required_Query
                Get_Optional_Query Set_Optional_Query )
        ],
        make_constructors => ["new"],
        make_pod          => {
            methods => [
                qw( get_required_query set_required_query
                    get_optional_query set_optional_query )
            ],
            synopsis    => $synopsis,
            constructor => { sample => $constructor },
        },
    },
}

__COPYRIGHT__

Copyright 2005-2009 Marvin Humphrey

This program is free software; you can redistribute it and/or modify
under the same terms as Perl itself.

