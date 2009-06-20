use Search::Kino03::KinoSearch;

1;

__END__

__AUTO_XS__

{   "Search::Kino03::KinoSearch::Search::TopDocs" => {
        bind_methods => [qw( 
            Get_Match_Docs
            Get_Total_Hits
            Set_Total_Hits
        )],
        make_constructors => ["new"],
    }
}


__COPYRIGHT__

Copyright 2005-2009 Marvin Humphrey

This program is free software; you can redistribute it and/or modify
under the same terms as Perl itself.

