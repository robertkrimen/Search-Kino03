use Search::Kino03::KinoSearch;

1;

__END__

__AUTO_XS__

my $synopsis = <<'END_SYNOPSIS';
    # Index-time.
    package MyDataWriter;
    use base qw( Search::Kino03::KinoSearch::Index::DataWriter );

    sub finish {
        my $self     = shift;
        my $segment  = $self->get_segment;
        my $metadata = $self->SUPER::metadata();
        $metadata->{foo} = $self->get_foo;
        $segment->store_metadata(
            key       => 'my_component',
            metadata  => $metadata
        );
    }

    # Search-time.
    package MyDataReader;
    use base qw( Search::Kino03::KinoSearch::Index::DataReader );

    sub new {
        my $self     = shift->SUPER::new(@_);
        my $segment  = $self->get_segment;
        my $metadata = $segment->fetch_metadata('my_component');
        if ($metadata) {
            $self->set_foo( $metadata->{foo} );
            ...
        }
        return $self;
    }
END_SYNOPSIS

{   "Search::Kino03::KinoSearch::Index::Segment" => {
        bind_methods => [
            qw(
                Add_Field
                _store_metadata|Store_Metadata
                Fetch_Metadata
                Field_Num
                Field_Name
                Get_Name
                Set_Count
                Get_Count
                Write_File
                Read_File
                )
        ],
        make_constructors => ["new"],
        make_pod          => {
            synopsis => $synopsis,
            methods  => [
                qw(
                    add_field
                    store_metadata
                    fetch_metadata
                    field_num
                    field_name
                    get_name
                    set_count
                    get_count
                    )
            ],
        },
    }
}

__COPYRIGHT__

Copyright 2005-2009 Marvin Humphrey

This program is free software; you can redistribute it and/or modify
under the same terms as Perl itself.

