use Search::Kino03::KinoSearch;

1;

__END__

__AUTO_XS__

my $synopsis = <<'END_SYNOPSIS';
    package MyQueryParser;
    use base qw( Search::Kino03::KinoSearch::QueryParser );

    sub expand_leaf {
        my ( $self, $leaf_query ) = @_;
        if ( $leaf_query->get_text =~ /.\*\s*$/ ) {
            return PrefixQuery->new(
                query_string => $leaf_query->get_text,
                field        => $leaf_query->get_field,
            );
        }
        else {
            return $self->SUPER::expand_leaf($leaf_query);
        }
    }
END_SYNOPSIS

my $constructor = <<'END_CONSTRUCTOR';
    my $leaf_query = Search::Kino03::KinoSearch::Search::LeafQuery->new(
        text  => '"three blind mice"',    # required
        field => 'content',               # default: undef
    );
END_CONSTRUCTOR

{   "Search::Kino03::KinoSearch::Search::LeafQuery" => {
        bind_methods => [qw( Get_Field Get_Text )],
        make_constructors => ["new"],
        make_pod          => {
            methods => [ qw( get_field get_text ) ],
            synopsis    => $synopsis,
            constructor => { sample => $constructor },
        }
    },
}

__COPYRIGHT__

Copyright 2008-2009 Marvin Humphrey

This program is free software; you can redistribute it and/or modify
under the same terms as Perl itself.

