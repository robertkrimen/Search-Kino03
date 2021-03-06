use strict;
use warnings;
use lib 'buildlib';

use Test::More tests => 2006;
use Search::Kino03::KinoSearch::Test::TestUtils qw( create_index );

my $folder = create_index( qw( a b c ), 'c c d' );
my $polyreader  = Search::Kino03::KinoSearch::Index::IndexReader->open( index => $folder );
my $reader      = $polyreader->get_seg_readers->[0];
my $post_reader = $reader->fetch("Search::Kino03::KinoSearch::Index::PostingsReader");

my $plist = $post_reader->posting_list( field => 'content', term => 'c' );

my ( @docs, @prox, @docs_from_next );
while ( my $doc_id = $plist->next ) {
    push @docs_from_next, $doc_id;
    my $posting = $plist->get_posting;
    push @docs, $posting->get_doc_id;
    push @prox, $posting->get_prox;
}
is_deeply( \@docs,           [ 3, 4 ], "correct docs after SegPList_Next" );
is_deeply( \@docs_from_next, [ 3, 4 ], "correct docs via SegPList_Next" );
is_deeply( \@prox, [ [0], [ 0, 1 ] ], "correct prox from SegPList_Next" );

$plist->seek('c');
$plist->next;
is( $plist->get_posting->get_doc_id, 3, "seek" );

$plist->set_doc_base(10);
$plist->seek('c');
my $doc_id = $plist->next;
is( $doc_id,                         13, "set_doc_base and next" );
is( $plist->get_posting->get_doc_id, 13, "set_doc_base" );
$plist->set_doc_base(0);

$folder = Search::Kino03::KinoSearch::Store::RAMFolder->new;

my $indexer = Search::Kino03::KinoSearch::Indexer->new(
    index  => $folder,
    schema => Search::Kino03::KinoSearch::Test::TestSchema->new,
);
for ( 0 .. 100 ) {
    my $content = 'a ';
    $content .= 'b ' if ( $_ % 2 == 0 );
    $content .= 'c ' if ( $_ % 3 == 0 );
    $content .= 'd ' if ( $_ % 4 == 0 );
    $content .= 'e ' if ( $_ % 5 == 0 );
    $indexer->add_doc( { content => $content } );
}
$indexer->commit;
$polyreader  = Search::Kino03::KinoSearch::Index::IndexReader->open( index => $folder );
$reader      = $polyreader->get_seg_readers->[0];
$post_reader = $reader->fetch("Search::Kino03::KinoSearch::Index::PostingsReader");

for my $letter (qw( a b c d e )) {
    my $skipping_plist = $post_reader->posting_list(
        field => 'content',
        term  => $letter,
    );
    my $plodding_plist = $post_reader->posting_list(
        field => 'content',
        term  => $letter,
    );

    # Compare results of advance() to results of next().
    for my $target ( 0 .. 99 ) {
        $skipping_plist->seek($letter);
        $plodding_plist->seek($letter);
        my $skipping_doc_id = $skipping_plist->advance($target);
        my $plodding_doc_id;
        do {
            $plodding_doc_id = $plodding_plist->next
                or die "shouldn't happen: $target";
        } while ( $plodding_plist->get_doc_id < $target );

        # Verify that the plists have identical state.
        is( $skipping_doc_id, $plodding_doc_id,
            "$letter: doc_ids via advance, next are identical" );
        is( $skipping_plist->get_doc_id, $plodding_plist->get_doc_id,
            "$letter: skip to $target" );
        is( $skipping_plist->get_post_stream->tell,
            $plodding_plist->get_post_stream->tell,
            "$letter: identical filepos for $target"
        );
        is( $skipping_plist->get_count, $plodding_plist->get_count,
            "$letter: identical count for $target" );
    }
}
