use Search::Kino03::KinoSearch;

1;

__END__

__AUTO_XS__

{   "Search::Kino03::KinoSearch::Test::Util::BBSortEx" => { make_constructors => ["new"], },
    "Search::Kino03::KinoSearch::Test::Util::BBSortExRun" => {
        make_constructors => ["new"],
        bind_methods      => [qw( Set_Mem_Thresh )],
    },
}

__COPYRIGHT__

Copyright 2005-2009 Marvin Humphrey

This program is free software; you can redistribute it and/or modify
under the same terms as Perl itself.

