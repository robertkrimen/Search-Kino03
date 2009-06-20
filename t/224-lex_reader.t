use strict;
use warnings;
use lib 'buildlib';

use Test::More tests => 3;
use Search::Kino03::KinoSearch::Test::TestUtils qw( create_index );

my $folder = create_index(
    "What's he building in there?",
    "What's he building in there?",
    "We have a right to know."
);
my $polyreader = Search::Kino03::KinoSearch::Index::IndexReader->open( index => $folder );
my $seg_reader = $polyreader->get_seg_readers->[0];
my $lex_reader = $seg_reader->obtain("Search::Kino03::KinoSearch::Index::LexiconReader");

my $lexicon = $lex_reader->lexicon( field => 'content', term => 'building' );
isa_ok( $lexicon, 'Search::Kino03::KinoSearch::Index::Lexicon',
    "lexicon returns a Search::Kino03::KinoSearch::Index::Lexicon" );
my $tinfo = $lexicon->get_term_info;
is( $tinfo->get_doc_freq, 2, "correct place in lexicon" );

$lexicon = $lex_reader->lexicon( field => 'content' );
$lexicon->next;
is( $lexicon->get_term, 'We',
    'calling lexicon without a term returns Lexicon with iterator reset' );

