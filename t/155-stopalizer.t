use strict;
use warnings;
use lib 'buildlib';

use Test::More tests => 6;
use Search::Kino03::KinoSearch::Test::TestUtils qw( test_analyzer );

my $stopalizer = Search::Kino03::KinoSearch::Analysis::Stopalizer->new( language => 'en' );
test_analyzer( $stopalizer, 'the', [], "single stopword stopalized" );

my $tokenizer    = Search::Kino03::KinoSearch::Analysis::Tokenizer->new;
my $polyanalyzer = Search::Kino03::KinoSearch::Analysis::PolyAnalyzer->new(
    analyzers => [ $tokenizer, $stopalizer ], );
test_analyzer( $polyanalyzer, 'i am the walrus',
    ['walrus'], "multiple stopwords stopalized" );
