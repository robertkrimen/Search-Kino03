use Search::Kino03::KinoSearch;

1;

__END__

__AUTO_XS__

my $synopsis = <<'END_SYNOPSIS';
    my $schema = Search::Kino03::KinoSearch::Schema->new;
    my $polyanalyzer = Search::Kino03::KinoSearch::Analysis::PolyAnalyzer->new( 
        language => 'en',
    );
    my $type = Search::Kino03::KinoSearch::FieldType::FullTextType->new(
        analyzer => $polyanalyzer,
    );
    $schema->spec_field( name => 'title',   type => $type );
    $schema->spec_field( name => 'content', type => $type );
END_SYNOPSIS

my $constructor = <<'END_CONSTRUCTOR';
    my $analyzer = Search::Kino03::KinoSearch::Analysis::PolyAnalyzer->new(
        language  => 'es',
    );
    
    # or...

    my $case_folder  = Search::Kino03::KinoSearch::Analysis::CaseFolder->new;
    my $tokenizer    = Search::Kino03::KinoSearch::Analysis::Tokenizer->new;
    my $stemmer      = Search::Kino03::KinoSearch::Analysis::Stemmer->new( language => 'en' );
    my $polyanalyzer = Search::Kino03::KinoSearch::Analysis::PolyAnalyzer->new(
        analyzers => [ $case_folder, $whitespace_tokenizer, $stemmer, ], );
END_CONSTRUCTOR

{   "Search::Kino03::KinoSearch::Analysis::PolyAnalyzer" => {
        make_constructors => ["new"],
        make_pod          => {
            methods     => [qw( get_analyzers )],
            synopsis    => $synopsis,
            constructor => { sample => $constructor },
        },
    },
}

__COPYRIGHT__

Copyright 2005-2009 Marvin Humphrey

This program is free software; you can redistribute it and/or modify
under the same terms as Perl itself.
