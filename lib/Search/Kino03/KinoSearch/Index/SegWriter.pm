use Search::Kino03::KinoSearch;

1;

__END__

__AUTO_XS__

{   "Search::Kino03::KinoSearch::Index::SegWriter" => {
        make_constructors => ["new"],
        bind_methods      => [
            qw(
                Add_Writer
                Register
                Fetch
                )
        ],
        make_pod => {
            methods => [
                qw(
                    add_doc
                    add_writer
                    register
                    fetch
                    )
            ],
        }
    }
}

__COPYRIGHT__

Copyright 2005-2009 Marvin Humphrey

This program is free software; you can redistribute it and/or modify
under the same terms as Perl itself.

