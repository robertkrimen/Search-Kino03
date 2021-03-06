use strict;
use warnings;
use lib 'buildlib';

use Test::More tests => 8;
use Search::Kino03::KinoSearch::Test;
use Search::Kino03::KinoSearch::Test::TestSchema;
use Search::Kino03::KinoSearch::Test::TestUtils qw( create_index );

my $folder_a = create_index( 'x a', 'x b', 'x c' );
my $folder_b = create_index( 'y b', 'y c', 'y d' );
my $searcher_a = Search::Kino03::KinoSearch::Searcher->new( index => $folder_a );
my $searcher_b = Search::Kino03::KinoSearch::Searcher->new( index => $folder_b );

my $poly_searcher = Search::Kino03::KinoSearch::Search::PolySearcher->new(
    schema      => Search::Kino03::KinoSearch::Test::TestSchema->new,
    searchables => [ $searcher_a, $searcher_b ],
);

is( $poly_searcher->doc_freq( field => 'content', term => 'b' ),
    2, 'doc_freq' );
is( $poly_searcher->doc_max, 6, 'doc_max' );
is( $poly_searcher->fetch_doc( doc_id => 1 )->{content}, 'x a', "fetch_doc" );
isa_ok( $poly_searcher->fetch_doc_vec(1), 'Search::Kino03::KinoSearch::Index::DocVector' );

my $hits = $poly_searcher->hits( query => 'a' );
is( $hits->total_hits, 1, "Find hit in first searcher" );

$hits = $poly_searcher->hits( query => 'd' );
is( $hits->total_hits, 1, "Find hit in second searcher" );

$hits = $poly_searcher->hits( query => 'c' );
is( $hits->total_hits, 2, "Find hits in both searchers" );

my $bit_vec
    = Search::Kino03::KinoSearch::Util::BitVector->new( capacity => $poly_searcher->doc_max );
my $bitcoll = Search::Kino03::KinoSearch::Search::HitCollector::BitCollector->new(
    bit_vector => $bit_vec );
my $query = $poly_searcher->glean_query('b');
$poly_searcher->collect( query => $query, collector => $bitcoll );
is_deeply( $bit_vec->to_arrayref, [ 2, 4 ], "collect" );
