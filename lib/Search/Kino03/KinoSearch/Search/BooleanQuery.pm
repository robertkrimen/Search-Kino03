use Carp;

confess( "BooleanQuery has been removed.  Its past functionality now resides "
        . "in ANDQUery, ORQuery, NOTQuery, and RequiredOptionalQuery." );

1;

__END__

__POD__

=head1 NAME

Search::Kino03::KinoSearch::Search::BooleanQuery - Removed.

=head1 DESCRIPTION 

BooleanQuery has been removed from KinoSearch as of version 0.30.  
Its past functionality can now be achieved using 
L<ANDQuery|Search::Kino03::KinoSearch::Search|ANDQuery>,
L<ORQuery|Search::Kino03::KinoSearch::Search|ORQuery>,
L<NOTQuery|Search::Kino03::KinoSearch::Search|NOTQuery>, and
L<RequiredOptionalQuery|Search::Kino03::KinoSearch::Search|RequiredOptionalQuery>,

=head1 COPYRIGHT

Copyright 2005-2009 Marvin Humphrey

=head1 LICENSE, DISCLAIMER, BUGS, etc.

See L<KinoSearch> version 0.30.

=cut

