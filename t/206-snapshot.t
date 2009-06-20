use strict;
use warnings;
use lib 'buildlib';

use Test::More tests => 1;
use Search::Kino03::KinoSearch::Test::TestUtils qw( create_index );

my $folder = create_index( "a", "a b" );

my $snapshot = Search::Kino03::KinoSearch::Index::Snapshot->new;
$snapshot->read_file( folder => $folder );
$snapshot->write_file( folder => $folder );

pass("successfully read and wrote snapshot without crashing");

