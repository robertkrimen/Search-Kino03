use strict;
use warnings;

package Search::Kino03::KSx::Index::ByteBufDocReader;
use base qw( Search::Kino03::KinoSearch::Index::DocReader );
use Search::Kino03::KinoSearch::Util::ByteBuf;
use Carp;

# Inside-out member vars.
our %width;
our %instream;

sub new {
    my ( $either, %args ) = @_;
    my $width = delete $args{width};
    my $self  = $either->SUPER::new(%args);
    confess("Missing required param 'width'") unless defined $width;
    if ( $width < 1 ) { confess("'width' must be at least 1") }
    $width{$$self} = $width;

    my $segment  = $self->get_segment;
    my $metadata = $self->get_segment->fetch_metadata("bytebufdocs");
    if ($metadata) {
        if ( $metadata->{format} != 1 ) {
            confess("Unrecognized format: '$metadata->{format}'");
        }
        my $filename = $segment->get_name . "/bytebufdocs.dat";
        $instream{$$self} = $self->get_folder->open_in($filename)
            or confess("Can't open '$filename'");
    }

    return $self;
}

sub fetch {
    my ( $self, %args ) = @_;
    my $doc_id = delete $args{doc_id};
    my $buf;
    $self->read_record( $doc_id, \$buf );
    if ($buf) {
        return Search::Kino03::KinoSearch::Util::ByteBuf->new($buf);
    }
    else {
        return undef;
    }
}

sub read_record {
    my ( $self, $doc_id, $buf ) = @_;
    my $instream = $instream{$$self};
    if ($instream) {
        my $width = $width{$$self};
        $instream->seek( $width * $doc_id );
        $instream->read_bytes( $$buf, $width );
    }
}

sub close {
    my $self = shift;
    delete $width{$$self};
    delete $instream{$$self};
}

sub DESTROY {
    my $self = shift;
    delete $width{$$self};
    delete $instream{$$self};
    $self->SUPER::DESTROY;
}

1;

__END__

__POD__

=head1 NAME

Search::Kino03::KSx::Index::ByteBufDocReader - Read a Doc as a fixed-width byte array.

=head1 SYNOPSIS

    # See Search::Kino03::KSx::Index::ByteBufDocWriter

=head1 DESCRIPTION

This is a proof-of-concept class to demonstrate alternate implementations for
fetching documents.  It is unsupported.

=head1 COPYRIGHT

Copyright 2009 Marvin Humphrey

=head1 LICENSE, DISCLAIMER, BUGS, etc.

See L<KinoSearch> version 0.30.

=cut

