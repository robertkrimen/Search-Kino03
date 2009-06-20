use Search::Kino03::KinoSearch;

1;

__END__

__AUTO_XS__


my $constructor = <<'END_CONSTRUCTOR';
    my $match_all_query = Search::Kino03::KinoSearch::Search::MatchAllQuery->new;
END_CONSTRUCTOR

{   "Search::Kino03::KinoSearch::Search::MatchAllQuery" => {
        make_constructors => ["new"],
                make_pod => {
            constructor => { sample => $constructor },
        }
    },
}

__COPYRIGHT__

Copyright 2005-2009 Marvin Humphrey

This program is free software; you can redistribute it and/or modify
under the same terms as Perl itself.

