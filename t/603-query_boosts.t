use strict;
use warnings;
use lib 'buildlib';

use Test::More tests => 2;
use Search::Kino03::KinoSearch::Test::TestUtils qw( create_index );

my $doc_1
    = 'a a a a a a a a a a a a a a a a a a a b c d x y ' . ( 'z ' x 100 );
my $doc_2 = 'a b c d x y x y ' . ( 'z ' x 100 );

my $folder = create_index( $doc_1, $doc_2 );
my $searcher = Search::Kino03::KinoSearch::Searcher->new( index => $folder );

my $a_query = Search::Kino03::KinoSearch::Search::TermQuery->new(
    field => 'content',
    term  => 'a',
);
my $x_y_query = Search::Kino03::KinoSearch::Search::PhraseQuery->new(
    field => 'content',
    terms => [qw( x y )],
);

my $combined_query
    = Search::Kino03::KinoSearch::Search::ORQuery->new( children => [ $a_query, $x_y_query ],
    );
my $hits = $searcher->hits( query => $combined_query );
my $hit = $hits->next;
is( $hit->{content}, $doc_1, "best doc ranks highest with no boosting" );

$x_y_query->set_boost(2);
$hits = $searcher->hits( query => $combined_query );
$hit = $hits->next;
is( $hit->{content}, $doc_2, "boosting a sub query succeeds" );
