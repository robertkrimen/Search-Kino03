use strict;
use warnings;
use lib 'buildlib';

use Test::More tests => 15;
use Search::Kino03::KinoSearch::Test::TestUtils qw( test_analyzer );

my $source_text = 'Eats, shoots and leaves.';

my $case_folder = Search::Kino03::KinoSearch::Analysis::CaseFolder->new;
my $tokenizer   = Search::Kino03::KinoSearch::Analysis::Tokenizer->new;
my $stopalizer  = Search::Kino03::KinoSearch::Analysis::Stopalizer->new( language => 'en' );
my $stemmer     = Search::Kino03::KinoSearch::Analysis::Stemmer->new( language => 'en' );

my $polyanalyzer
    = Search::Kino03::KinoSearch::Analysis::PolyAnalyzer->new( analyzers => [], );
test_analyzer( $polyanalyzer, $source_text, [$source_text],
    'no sub analyzers' );

$polyanalyzer
    = Search::Kino03::KinoSearch::Analysis::PolyAnalyzer->new( analyzers => [$case_folder], );
test_analyzer(
    $polyanalyzer, $source_text,
    ['eats, shoots and leaves.'],
    'with CaseFolder'
);

$polyanalyzer = Search::Kino03::KinoSearch::Analysis::PolyAnalyzer->new(
    analyzers => [ $case_folder, $tokenizer ], );
test_analyzer(
    $polyanalyzer, $source_text,
    [ 'eats', 'shoots', 'and', 'leaves' ],
    'with Tokenizer'
);

$polyanalyzer = Search::Kino03::KinoSearch::Analysis::PolyAnalyzer->new(
    analyzers => [ $case_folder, $tokenizer, $stopalizer ], );
test_analyzer(
    $polyanalyzer, $source_text,
    [ 'eats', 'shoots', 'leaves' ],
    'with Stopalizer'
);

$polyanalyzer = Search::Kino03::KinoSearch::Analysis::PolyAnalyzer->new(
    analyzers => [ $case_folder, $tokenizer, $stopalizer, $stemmer, ], );
test_analyzer( $polyanalyzer, $source_text, [ 'eat', 'shoot', 'leav' ],
    'with Stemmer' );

