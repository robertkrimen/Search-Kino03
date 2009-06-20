use Search::Kino03::KinoSearch;

1;

__END__

__AUTO_XS__

my $synopsis = <<'END_SYNOPSIS';
    use Search::Kino03::KinoSearch::Schema;
    use Search::Kino03::KinoSearch::FieldType::FullTextType;
    use Search::Kino03::KinoSearch::Analysis::PolyAnalyzer;
    
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
    my $schema = MySchema->new;
END_CONSTRUCTOR

{   "Search::Kino03::KinoSearch::Schema" => {
        bind_methods => [
            qw(
                Architecture
                Get_Architecture
                Get_Similarity
                Fetch_Type
                Fetch_Analyzer
                Fetch_Sim
                Fetch_Posting
                Num_Fields
                All_Fields
                Spec_Field
                Write
                Eat
                )
        ],
        make_constructors => [qw( new )],
        make_pod          => {
            methods => [
                qw(
                    spec_field
                    num_fields
                    all_fields
                    fetch_type
                    fetch_sim
                    architecture
                    get_architecture
                    get_similarity
                    )
            ],
            synopsis     => $synopsis,
            constructors => [ { sample => $constructor } ],
        },
    }
}

__COPYRIGHT__

Copyright 2005-2009 Marvin Humphrey

This program is free software; you can redistribute it and/or modify
under the same terms as Perl itself.

