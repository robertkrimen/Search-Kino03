use Search::Kino03::KinoSearch;

1;

__END__

__AUTO_XS__

my $synopsis = <<'END_SYNOPSIS';
    my $stopalizer = Search::Kino03::KinoSearch::Analysis::Stopalizer->new(
        language => 'fr',
    );
    my $polyanalyzer = Search::Kino03::KinoSearch::Analysis::PolyAnalyzer->new(
        analyzers => [ $case_folder, $tokenizer, $stopalizer, $stemmer ],
    );

This class uses Lingua::StopWords for its default stoplists, so it supports
the same set of languages.
END_SYNOPSIS

my $constructor = <<'END_CONSTRUCTOR';
    my $stopalizer = Search::Kino03::KinoSearch::Analysis::Stopalizer->new(
        language => 'de',
    );
    
    # or...
    my $stopalizer = Search::Kino03::KinoSearch::Analysis::Stopalizer->new(
        stoplist => \%stoplist,
    );
END_CONSTRUCTOR

{   "Search::Kino03::KinoSearch::Analysis::Stopalizer" => {
        make_constructors => ["new"],
        make_pod          => {
            synopsis    => $synopsis,
            constructor => { sample => $constructor }
        },
    },
}

__COPYRIGHT__

Copyright 2005-2009 Marvin Humphrey

This program is free software; you can redistribute it and/or modify
under the same terms as Perl itself

