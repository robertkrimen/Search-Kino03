use Search::Kino03::KinoSearch;

1;

__END__

__AUTO_XS__

my $synopsis = <<'END_SYNOPSIS';
    my $case_folder = Search::Kino03::KinoSearch::Analysis::CaseFolder->new;

    my $polyanalyzer = Search::Kino03::KinoSearch::Analysis::PolyAnalyzer->new(
        analyzers => [ $case_folder, $tokenizer, $stemmer ],
    );
END_SYNOPSIS

my $constructor = <<'END_CONSTRUCTOR';
    my $case_folder = Search::Kino03::KinoSearch::Analysis::CaseFolder->new;
END_CONSTRUCTOR

{   "Search::Kino03::KinoSearch::Analysis::CaseFolder" => {
        make_constructors => ["new"],
        make_pod          => {
            synopsis    => $synopsis,
            constructor => { sample => $constructor },
        }
    },
}

__COPYRIGHT__

Copyright 2005-2009 Marvin Humphrey

This program is free software; you can redistribute it and/or modify
under the same terms as Perl itself.

