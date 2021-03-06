use strict;
use warnings;

use lib 'buildlib';
use Search::Kino03::KinoSearch::Test;

package NonMergingIndexManager;
use base qw( Search::Kino03::KinoSearch::Index::IndexManager );

sub segreaders_to_merge {
    return Search::Kino03::KinoSearch::Util::VArray->new( capacity => 0 );
}

# BiggerSchema is like TestSchema, but it has an extra field named "aux".
# Because "aux" sorts before "content", it forces a remapping of field numbers
# when an index created under TestSchema is opened/modified under
# BiggerSchema.
package BiggerSchema;
use base qw( Search::Kino03::KinoSearch::Test::TestSchema );

sub new {
    my $self = shift->SUPER::new(@_);
    my $type = Search::Kino03::KinoSearch::FieldType::FullTextType->new(
        analyzer      => Search::Kino03::KinoSearch::Analysis::Tokenizer->new,
        highlightable => 1,
    );
    $self->spec_field( name => 'content', type => $type );
    $self->spec_field( name => 'aux',     type => $type );
    return $self;
}

package main;
use Test::More tests => 8;
use Search::Kino03::KinoSearch::Test::TestSchema;
use Search::Kino03::KinoSearch::Test::TestUtils qw( create_index init_test_index_loc );
use File::Find qw( find );

my $index_loc = init_test_index_loc();

my $num_reps;
{
    # Verify that optimization truly cuts down on the number of segments.
    my $schema = Search::Kino03::KinoSearch::Test::TestSchema->new;
    for ( $num_reps = 1;; $num_reps++ ) {
        my $indexer = Search::Kino03::KinoSearch::Indexer->new(
            index  => $index_loc,
            schema => $schema,
        );
        my $num_cf_files = num_cf_files($index_loc);
        if ( $num_reps > 2 and $num_cf_files > 1 ) {
            $indexer->optimize;
            $indexer->commit;
            $num_cf_files = num_cf_files($index_loc);
            is( $num_cf_files, 1, 'commit after optimize' );
            last;
        }
        else {
            $indexer->add_doc( { content => $_ } ) for 1 .. 5;
            $indexer->commit;
        }
    }
}

init_test_index_loc();

my @correct;
for my $num_letters ( reverse 1 .. 10 ) {
    my $truncate = $num_letters == 10 ? 1 : 0;
    my $indexer = Search::Kino03::KinoSearch::Indexer->new(
        schema   => Search::Kino03::KinoSearch::Test::TestSchema->new,
        index    => $index_loc,
        truncate => $truncate,
    );

    for my $letter ( 'a' .. 'b' ) {
        my $content = ( "$letter " x $num_letters ) . ( 'z ' x 50 );
        $indexer->add_doc( { content => $content } );
        push @correct, $content if $letter eq 'b';
    }
    $indexer->commit;
}

{
    my $searcher = Search::Kino03::KinoSearch::Searcher->new( index => $index_loc );
    my $hits = $searcher->hits( query => 'b' );
    is( $hits->total_hits, 10, "correct total_hits from merged index" );
    my @got;
    push @got, $hits->next->{content} for 1 .. $hits->total_hits;
    is_deeply( \@got, \@correct,
        "correct top scoring hit from merged index" );
}

{
    # Reopen index under BiggerSchema and add some content.
    my $schema  = BiggerSchema->new;
    my $folder  = Search::Kino03::KinoSearch::Store::FSFolder->new( path => $index_loc );
    my $indexer = Search::Kino03::KinoSearch::Indexer->new(
        schema  => $schema,
        index   => $folder,
        manager => NonMergingIndexManager->new( folder => $folder ),
    );
    $indexer->add_doc( { aux => 'foo', content => 'bar' } );

    # Now add some indexes.
    my $another_folder = create_index( "atlantic ocean", "fresh fish" );
    my $yet_another_folder = create_index("bonus");
    $indexer->add_index($another_folder);
    $indexer->add_index($yet_another_folder);
    $indexer->commit;
    cmp_ok( num_cf_files($index_loc),
        '>', 1, "non-merging Indexer should produce multi-seg index" );
}

{
    my $searcher = Search::Kino03::KinoSearch::Searcher->new( index => $index_loc );
    my $hits = $searcher->hits( query => 'fish' );
    is( $hits->total_hits, 1, "correct total_hits after add_index" );
    is( $hits->next->{content},
        'fresh fish', "other indexes successfully absorbed" );
}

{
    # Open an IndexReader, to prevent the deletion of files on Windows and
    # verify the file purging mechanism.
    my $schema  = BiggerSchema->new;
    my $folder  = Search::Kino03::KinoSearch::Store::FSFolder->new( path => $index_loc );
    my $reader  = Search::Kino03::KinoSearch::Index::IndexReader->open( index => $folder );
    my $indexer = Search::Kino03::KinoSearch::Indexer->new(
        schema => $schema,
        index  => $folder,
    );
    $indexer->optimize;
    $indexer->commit;
    $reader->close;
    undef $reader;
    $indexer = Search::Kino03::KinoSearch::Indexer->new(
        schema => $schema,
        index  => $folder,
    );
    $indexer->optimize;
    $indexer->commit;
}

is( num_cf_files($index_loc), 1,
    "merged segment files successfully deleted" );

is( Search::Kino03::KinoSearch::Store::FileDes::object_count(),
    0, "All FileDes objects have been cleaned up" );

sub num_cf_files {
    my $dir          = shift;
    my $num_cf_files = 0;
    find(
        {   no_chdir => 1,
            wanted =>
                sub { $num_cf_files++ if $File::Find::name =~ /cf\.dat/ },
        },
        $dir,
    );
    return $num_cf_files;
}
