use strict;
use warnings;

package Search::Kino03::KSx::Remote::SearchServer;
BEGIN { our @ISA = qw( Search::Kino03::KinoSearch::Obj ) }
use Carp;
use Storable qw( nfreeze thaw );
use bytes;
no bytes;

# Inside-out member vars.
our %searchable;
our %port;
our %password;
our %sock;

use IO::Socket::INET;
use IO::Select;

sub new {
    my ( $either, %args ) = @_;
    my $searchable = delete $args{searchable};
    my $password   = delete $args{password};
    my $port       = delete $args{port};
    my $self       = $either->SUPER::new(%args);
    $searchable{$$self} = $searchable;
    confess("Missing required param 'password'") unless defined $password;
    $password{$$self} = $password;

    # Establish a listening socket.
    $port{$$self} = $port;
    confess("Invalid port: $port") unless $port =~ /^\d+$/;
    my $sock = IO::Socket::INET->new(
        LocalPort => $port,
        Proto     => 'tcp',
        Listen    => SOMAXCONN,
        Reuse     => 1,
    );
    confess("No socket: $!") unless $sock;
    $sock->autoflush(1);
    $sock{$$self} = $sock;

    return $self;
}

sub DESTROY {
    my $self = shift;
    delete $searchable{$$self};
    delete $port{$$self};
    delete $password{$$self};
    delete $sock{$$self};
    $self->SUPER::DESTROY;
}

my %dispatch = (
    doc_max       => \&do_doc_max,
    doc_freq      => \&do_doc_freq,
    top_docs      => \&do_top_docs,
    fetch_doc     => \&do_fetch_doc,
    fetch_doc_vec => \&do_fetch_doc_vec,
    terminate     => undef,
);

sub serve {
    my $self      = shift;
    my $main_sock = $sock{$$self};
    my $read_set  = IO::Select->new($main_sock);

    while ( my @ready = $read_set->can_read ) {
        for my $readhandle (@ready) {
            # If this is the main handle, we have a new client, so accept.
            if ( $readhandle == $main_sock ) {
                my $client_sock = $main_sock->accept;

                # Verify password.
                my $pass = <$client_sock>;
                chomp($pass) if defined $pass;
                if ( defined $pass && $pass eq $password{$$self} ) {
                    $read_set->add($client_sock);
                    print $client_sock "accepted\n";
                }
                else {
                    print $client_sock "password incorrect\n";
                }
            }
            # Otherwise it's a client sock, so process the request.
            else {
                my $client_sock = $readhandle;
                my ( $check_val, $buf, $len, $method, $args );
                chomp( $method = <$client_sock> );

                # If "done", the client's closing.
                if ( $method eq 'done' ) {
                    $read_set->remove($client_sock);
                    $client_sock->close;
                    next;
                }
                # Remote signal to close the server.
                elsif ( $method eq 'terminate' ) {
                    $read_set->remove($client_sock);
                    $client_sock->close;
                    $main_sock->close;
                    return;
                }
                # Sanity check the method name.
                elsif ( !$dispatch{$method} ) {
                    print $client_sock "ERROR: Bad method name: $method\n";
                    next;
                }

                # Process the method call.
                read( $client_sock, $buf, 4 );
                $len = unpack( 'N', $buf );
                read( $client_sock, $buf, $len );
                my $response   = $dispatch{$method}->( $self, thaw($buf) );
                my $frozen     = nfreeze($response);
                my $packed_len = pack( 'N', bytes::length($frozen) );
                print $client_sock $packed_len . $frozen;
            }
        }
    }
}

sub do_doc_freq {
    my ( $self, $args ) = @_;
    return { retval => $searchable{$$self}->doc_freq(%$args) };
}

sub do_top_docs {
    my ( $self, $args ) = @_;
    my $top_docs = $searchable{$$self}->top_docs(%$args);
    return { retval => $top_docs };
}

sub do_doc_max {
    my ( $self, $args ) = @_;
    my $doc_max = $searchable{$$self}->doc_max;
    return { retval => $doc_max };
}

sub do_fetch_doc {
    my ( $self, $args ) = @_;
    my $doc = $searchable{$$self}->fetch_doc(%$args);
    return { retval => $doc };
}

sub do_fetch_doc_vec {
    my ( $self, $args ) = @_;
    my $doc_vec = $searchable{$$self}->fetch_doc_vec( $args->{doc_id} );
    return { retval => $doc_vec };
}

1;

__END__

=head1 NAME

Search::Kino03::KSx::Remote::SearchServer - Make a Searcher remotely accessible.

=head1 SYNOPSIS

    my $searcher = Search::Kino03::KinoSearch::Searcher->new( index => '/path/to/index' );
    my $search_server = Search::Kino03::KSx::Remote::SearchServer->new(
        searchable => $searcher,
        port       => 7890,
        password   => $pass,
    );
    $search_server->serve;

=head1 DESCRIPTION 

The SearchServer class, in conjunction with
L<SearchClient|Search::Kino03::KSx::Remote::SearchClient>, makes it possible to run
a search on one machine and report results on another.  

By aggregating several SearchClients under a
L<PolySearcher|Search::Kino03::KinoSearch::Search::PolySearcher>, the cost of searching
what might have been a prohibitively large monolithic index can be distributed
across multiple nodes, each with its own, smaller index.

=head1 METHODS

=head2 new

    my $search_server = Search::Kino03::KSx::Remote::SearchServer->new(
        searchable => $searcher, # required
        port       => 7890,      # required
        password   => $pass,     # required
    );

Constructor.  Takes hash-style parameters.

=over

=item *

B<searchable> - the L<Searcher|Search::Kino03::KinoSearch::Searcher> that the SearchServer
will wrap.

=item *

B<port> - the port on localhost that the server should open and listen on.

=item *

B<password> - a password which must be supplied by clients.

=back

=head2 serve

    $search_server->serve;

Open a listening socket on localhost and wait for SearchClients to connect.

=head1 COPYRIGHT

Copyright 2006-2009 Marvin Humphrey

=head1 LICENSE, DISCLAIMER, BUGS, etc.

See L<KinoSearch> version 0.30.

=cut
