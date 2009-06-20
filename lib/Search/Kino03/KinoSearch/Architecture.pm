use Search::Kino03::KinoSearch;

1;

__END__

__AUTO_XS__

my $synopsis = <<'END_SYNOPSIS';
    package MyArchitecture;
    use base qw( Search::Kino03::KinoSearch::Architecture );

    use Search::Kino03::KSx::Index::ZlibDocWriter;
    use Search::Kino03::KSx::Index::ZlibDocReader;

    sub register_doc_writer {
        my ( $self, $seg_writer ) = @_; 
        my $doc_writer = Search::Kino03::KSx::Index::ZlibDocWriter->new(
            snapshot   => $seg_writer->get_snapshot,
            segment    => $seg_writer->get_segment,
            polyreader => $seg_writer->get_polyreader,
        );  
        $seg_writer->register(
            api       => "Search::Kino03::KinoSearch::Index::DocReader",
            component => $doc_writer,
        );  
        $seg_writer->add_writer($doc_writer);
    }

    sub register_doc_reader {
        my ( $self, $seg_reader ) = @_; 
        my $doc_reader = Search::Kino03::KSx::Index::ZlibDocReader->new(
            schema   => $seg_reader->get_schema,
            folder   => $seg_reader->get_folder,
            segments => $seg_reader->get_segments,
            seg_tick => $seg_reader->get_seg_tick,
            snapshot => $seg_reader->get_snapshot,
        );  
        $seg_reader->register(
            api       => 'Search::Kino03::KinoSearch::Index::DocReader',
            component => $doc_reader,
        );  
    }
 
    package MySchema;
    use base qw( Search::Kino03::KinoSearch::Schema );
    
    sub architecture { 
        shift;
        return MyArchitecture->new(@_); 
    }
END_SYNOPSIS

{   "Search::Kino03::KinoSearch::Architecture" => {
        bind_methods => [
            qw(
                Index_Interval
                Skip_Interval
                Init_Seg_Reader
                Register_Doc_Writer
                Register_Doc_Reader
                Register_Deletions_Writer
                Register_Deletions_Reader
                Register_Lexicon_Reader
                Register_Postings_Writer
                Register_Postings_Reader
                Register_Sort_Writer
                Register_Sort_Reader
                Register_Highlight_Writer
                Register_Highlight_Reader
                Make_Similarity
                )
        ],
        make_constructors => ["new"],
        make_pod          => {
            synopsis => $synopsis,
            methods  => [
                qw(
                    register_doc_writer
                    register_doc_reader
                    )
            ],
        }
    }
}

__COPYRIGHT__

Copyright 2005-2009 Marvin Humphrey

This program is free software; you can redistribute it and/or modify
under the same terms as Perl itself.

