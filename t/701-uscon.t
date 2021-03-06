use strict;
use warnings;
use lib 'buildlib';

use Test::More tests => 9;
use Search::Kino03::KinoSearch::Test::TestUtils qw( persistent_test_index_loc );
use Search::Kino03::KinoSearch::Test::USConSchema;

my $searcher
    = Search::Kino03::KinoSearch::Searcher->new( index => persistent_test_index_loc() );
isa_ok( $searcher, 'Search::Kino03::KinoSearch::Searcher' );

my %searches = (
    'United'              => 34,
    'shall'               => 50,
    'not'                 => 27,
    '"shall not"'         => 21,
    'shall not'           => 51,
    'Congress'            => 31,
    'Congress AND United' => 22,
    '(Congress AND United) OR ((Vice AND President) OR "free exercise")' =>
        28,
);

while ( my ( $qstring, $num_expected ) = each %searches ) {
    my $hits = $searcher->hits( query => $qstring );
    is( $hits->total_hits, $num_expected, $qstring );
}

