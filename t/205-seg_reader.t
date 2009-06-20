use strict;
use warnings;
use lib 'buildlib';

use Test::More tests => 8;
use Search::Kino03::KinoSearch::Test::TestUtils qw( create_index );

my $folder = create_index(
    "What's he building in there?",
    "What's he building in there?",
    "We have a right to know."
);
my $polyreader = Search::Kino03::KinoSearch::Index::IndexReader->open( index => $folder );
my $reader = $polyreader->get_seg_readers->[0];

isa_ok( $reader, 'Search::Kino03::KinoSearch::Index::SegReader' );

is( $reader->doc_max, 3, "doc_max returns correct number" );

my $lex_reader = $reader->fetch("Search::Kino03::KinoSearch::Index::LexiconReader");
isa_ok(
    $lex_reader,
    'Search::Kino03::KinoSearch::Index::LexiconReader',
    "fetch() a component"
);
ok( !defined( $reader->fetch("nope") ),
    "fetch() returns undef when component can't be found" );
$lex_reader = $reader->obtain("Search::Kino03::KinoSearch::Index::LexiconReader");
isa_ok(
    $lex_reader,
    'Search::Kino03::KinoSearch::Index::LexiconReader',
    "obtain() a component"
);
eval { $reader->obtain("boom."); };
like( $@, qr/boom/, "obtain blows up when component can't be found" );

is_deeply( $reader->seg_readers, [$reader], "seg_readers" );
is_deeply( $reader->offsets,     [0],       "offsets" );

