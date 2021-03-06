use Search::Kino03::KinoSearch;

1;

__END__

__AUTO_XS__

my $synopsis = <<'END_SYNOPSIS';
    # MatchPosting is used indirectly, by specifying in FieldType subclass.
    package MySchema::Category;
    use base qw( Search::Kino03::KinoSearch::FieldType::FullTextType );
    sub posting {
        my $self = shift;
        return Search::Kino03::KinoSearch::Posting::MatchPosting->new(@_);
    }
END_SYNOPSIS

{   "Search::Kino03::KinoSearch::Posting::MatchPosting" => {
        make_constructors  => ["new"],
        make_getters       => [qw( freq )],
#        make_pod => {
#            synopsis => $synopsis,
#        }
    }
}

__COPYRIGHT__

Copyright 2005-2009 Marvin Humphrey

This program is free software; you can redistribute it and/or modify
under the same terms as Perl itself.

