use strict;
use warnings;
use lib 'buildlib';

use Test::More tests => 14;
use Search::Kino03::KinoSearch::Test::TestSchema;
use Search::Kino03::KinoSearch::Test::TestUtils qw( create_index );

my $folder     = create_index( 'a' .. 'e' );
my $polyreader = Search::Kino03::KinoSearch::Index::PolyReader->open( index => $folder, );
my $seg_reader = $polyreader->seg_readers->[0];
my $snapshot   = $polyreader->get_snapshot;

my $del_writer = Search::Kino03::KinoSearch::Index::DefaultDeletionsWriter->new(
    polyreader => $polyreader,
    segment    => $seg_reader->get_segment,
    snapshot   => $snapshot,
);
$del_writer->delete_by_term( field => 'content', term => 'c' );
my $doc_map = $del_writer->generate_doc_map(
    deletions => $del_writer->seg_deletions($seg_reader),
    doc_max   => $seg_reader->doc_max,
    offset    => 0,
);
my @correct = ( 1, 2, 0, 3, 4 );
my @got;
push @got, $doc_map->get($_) for 1 .. 5;
is_deeply( \@got, \@correct, "doc map maps around deleted docs" );

$doc_map = $del_writer->generate_doc_map(
    deletions => $del_writer->seg_deletions($seg_reader),
    doc_max   => $seg_reader->doc_max,
    offset    => 100,
);
is( $doc_map->get(4), 103, "doc map handles offset correctly" );
ok( !$doc_map->get(3), "doc_map handled deletions correctly" );

my $new_seg = Search::Kino03::KinoSearch::Index::Segment->new(
    folder => $folder,
    name   => 'seg_2',
);
$del_writer = Search::Kino03::KinoSearch::Index::DefaultDeletionsWriter->new(
    polyreader => $polyreader,
    segment    => $new_seg,
    snapshot   => $snapshot,
);
$del_writer->delete_by_term( field => 'content', term => 'a' );
$del_writer->delete_by_term( field => 'content', term => 'b' );
$del_writer->finish;
$new_seg->write_file;
$snapshot->add_entry( $new_seg->get_name . "/segmeta.json" );

for my $entry ( values %{ $new_seg->fetch_metadata('deletions')->{files} } ) {
    $snapshot->add_entry( $entry->{filename} );
}
$snapshot->write_file( folder => $folder );

$polyreader = Search::Kino03::KinoSearch::Index::PolyReader->open( index => $folder );
$seg_reader = $polyreader->seg_readers->[0];
my $del_reader = $seg_reader->obtain("Search::Kino03::KinoSearch::Index::DeletionsReader");
my $deldocs    = $del_reader->read_deletions;

my @deleted_or_not = map { $deldocs->get($_) } 0 .. 7;
is_deeply(
    \@deleted_or_not,
    [ 0, 1, 1, 0, 0, 0, 0, 0 ],
    "finish() and read_deldocs() save/recover deletions correctly"
);

is( $deldocs->count, 2,
    "finish() and read_deldocs() save/recover num_deletions correctly" );
is( $deldocs->get_cap, 8, "finish() wrote correct number of bytes" );

$folder = Search::Kino03::KinoSearch::Store::RAMFolder->new;
my $schema  = Search::Kino03::KinoSearch::Test::TestSchema->new;
my $indexer = Search::Kino03::KinoSearch::Indexer->new(
    index  => $folder,
    schema => $schema,
);
$indexer->add_doc( { content => $_ } ) for 'a' .. 'c';
$indexer->commit;
$indexer = Search::Kino03::KinoSearch::Indexer->new(
    index  => $folder,
    schema => $schema,
);
$indexer->delete_by_query( Search::Kino03::KinoSearch::Search::MatchAllQuery->new );
$indexer->commit;
$indexer = Search::Kino03::KinoSearch::Indexer->new(
    index  => $folder,
    schema => $schema,
);
$indexer->add_doc( { content => $_ } ) for 'a' .. 'c';
$indexer->commit;

my $searcher = Search::Kino03::KinoSearch::Searcher->new( index => $folder );
my $hits = $searcher->hits( query => 'a' );
is( $hits->total_hits, 1, "deleting then re-adding works" );

my @expected;
for ( 'a' .. 'e' ) {
    $hits = $searcher->hits( query => $_ );
    my @contents;
    while ( my $hit = $hits->next ) {
        push @contents, $hit->{content};
    }
    push @expected, \@contents;
}
$indexer = Search::Kino03::KinoSearch::Indexer->new(
    index  => $folder,
    schema => $schema,
);
$indexer->optimize;
$indexer->commit;
$searcher = Search::Kino03::KinoSearch::Searcher->new( index => $folder );
@got = ();
for ( 'a' .. 'e' ) {
    $hits = $searcher->hits( query => $_ );
    my @contents;
    while ( my $hit = $hits->next ) {
        push @contents, $hit->{content};
    }
    push @got, \@contents;
}
is_deeply( \@got, \@expected, "segment merging handles deletions correctly" );

$indexer = Search::Kino03::KinoSearch::Indexer->new(
    index  => $folder,
    schema => $schema,
);
$indexer->delete_by_term( field => 'content', term => $_ ) for 'a' .. 'c';
$indexer->commit;
$searcher = Search::Kino03::KinoSearch::Searcher->new( index => $folder );
$hits = $searcher->hits( query => 'a' );
is( $hits->total_hits, 0, "adding and searching empty segments is ok" );

$indexer = Search::Kino03::KinoSearch::Indexer->new(
    index    => $folder,
    schema   => $schema,
    truncate => 1,
);
$indexer->add_doc( { content => 'foo' } );
$indexer->add_doc( { content => 'bar' } );
$indexer->commit;

$searcher = Search::Kino03::KinoSearch::Searcher->new( index => $folder );
is( $searcher->doc_max, 2, "correct number of docs in index" );
$hits = $searcher->hits( query => 'foo' );
is( $hits->total_hits, 1, "found term" );

$indexer = Search::Kino03::KinoSearch::Indexer->new(
    index    => $folder,
    schema   => $schema,
    truncate => 1,
);
$indexer->add_doc( { content => 'baz' } );
$indexer->commit;
$searcher = Search::Kino03::KinoSearch::Searcher->new( index => $folder );
is( $searcher->doc_max, 1, "correct doc_max after truncation" );
$hits = $searcher->hits( query => 'foo' );
is( $hits->total_hits, 0, "truncate succeeded" );
$hits = $searcher->hits( query => 'baz' );
is( $hits->total_hits, 1, "added doc during same session as truncation" );
