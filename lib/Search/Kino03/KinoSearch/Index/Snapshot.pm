use Search::Kino03::KinoSearch;

1;

__END__

__AUTO_XS__

my $synopsis = <<'END_SYNOPSIS';
    my $snapshot = Search::Kino03::KinoSearch::Index::Snapshot->new;
    $snapshot->read_file( folder => $folder );    # load most recent snapshot
    my $files = $snapshot->list;
    print "$_\n" for @$files;
END_SYNOPSIS

my $constructor = <<'END_CONSTRUCTOR';
    my $snapshot = Search::Kino03::KinoSearch::Index::Snapshot->new;
END_CONSTRUCTOR

{   "Search::Kino03::KinoSearch::Index::Snapshot" => {
        bind_methods => [
            qw(
                List 
                Num_Entries
                Add_Entry
                Delete_Entry
                Read_File
                Write_File
                Get_Filename
                )
        ],
        make_constructors => ["new"],
        make_pod          => {
            synopsis => $synopsis,
            constructor => { sample => $constructor },
            methods  => [
                qw(
                    list 
                    num_entries
                    add_entry
                    delete_entry
                    read_file
                    write_file
                    get_filename
                    )
            ],
        },
    }
}

__COPYRIGHT__

Copyright 2005-2009 Marvin Humphrey

This program is free software; you can redistribute it and/or modify
under the same terms as Perl itself.

