use strict;
use warnings;
use lib 'buildlib';

use Test::More tests => 6;
use Search::Kino03::KinoSearch::Test::TestUtils qw( test_analyzer );

my $stemmer = Search::Kino03::KinoSearch::Analysis::Stemmer->new( language => 'en' );
test_analyzer( $stemmer, 'peas', ['pea'], "single word stemmed" );

my $tokenizer    = Search::Kino03::KinoSearch::Analysis::Tokenizer->new;
my $polyanalyzer = Search::Kino03::KinoSearch::Analysis::PolyAnalyzer->new(
    analyzers => [ $tokenizer, $stemmer ], );
test_analyzer(
    $polyanalyzer,
    'peas porridge hot',
    [ 'pea', 'porridg', 'hot' ],
    "multiple words stemmed",
);
