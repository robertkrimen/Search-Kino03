use Search::Kino03::KinoSearch;

1;

__END__

__AUTO_XS__

my $synopsis = <<'END_SYNOPSIS';
    my $postings_reader 
        = $seg_reader->obtain("Search::Kino03::KinoSearch::Index::PostingsReader");
    my $posting_list = $postings_reader->posting_list(
        field => 'title', 
        term  => 'foo',
    );
END_SYNOPSIS

{   "Search::Kino03::KinoSearch::Index::PostingsReader" => {
        make_constructors => ["new"],
        bind_methods      => [qw( Posting_List Get_Lex_Reader )],
        make_pod          => {
            synopsis => $synopsis,
            methods  => [qw( posting_list )],
        },
    },
    "Search::Kino03::KinoSearch::Index::DefaultPostingsReader" => {
        make_constructors => ["new"],
    }
}

__COPYRIGHT__

Copyright 2005-2009 Marvin Humphrey

This program is free software; you can redistribute it and/or modify
under the same terms as Perl itself.
