use strict;
use warnings;

package MyArchitecture;
use base qw( Search::Kino03::KinoSearch::Architecture );

use Search::Kino03::KSx::Index::ByteBufDocWriter;
use Search::Kino03::KSx::Index::ByteBufDocReader;

sub register_doc_writer {
    my ( $self, $seg_writer ) = @_;
    my $doc_writer = Search::Kino03::KSx::Index::ByteBufDocWriter->new(
        width      => 1,
        field      => 'id',
        snapshot   => $seg_writer->get_snapshot,
        segment    => $seg_writer->get_segment,
        polyreader => $seg_writer->get_polyreader,
    );
    $seg_writer->register(
        api       => "Search::Kino03::KinoSearch::Index::DocReader",
        component => $doc_writer,
    );
    $seg_writer->add_writer($doc_writer);
}

sub register_doc_reader {
    my ( $self, $seg_reader ) = @_;
    my $doc_reader = Search::Kino03::KSx::Index::ByteBufDocReader->new(
        width    => 1,
        schema   => $seg_reader->get_schema,
        folder   => $seg_reader->get_folder,
        segments => $seg_reader->get_segments,
        seg_tick => $seg_reader->get_seg_tick,
        snapshot => $seg_reader->get_snapshot,
    );
    $seg_reader->register(
        api       => 'Search::Kino03::KinoSearch::Index::DocReader',
        component => $doc_reader,
    );
}

package MySchema;
use base qw( Search::Kino03::KinoSearch::Schema );

sub architecture { MyArchitecture->new }

sub new {
    my $self      = shift->SUPER::new(@_);
    my $tokenizer = Search::Kino03::KinoSearch::Analysis::Tokenizer->new;
    my $type
        = Search::Kino03::KinoSearch::FieldType::FullTextType->new( analyzer => $tokenizer );
    $self->spec_field( name => 'id', type => $type );
    return $self;
}

package main;
use Test::More tests => 4;

my $folder = Search::Kino03::KinoSearch::Store::RAMFolder->new;
my $schema = MySchema->new;

sub add_to_index {
    my $indexer = Search::Kino03::KinoSearch::Indexer->new(
        index  => $folder,
        schema => $schema,
    );
    $indexer->add_doc( { id => $_ } ) for @_;
    $indexer->commit;
}

add_to_index(qw( a b c ));

my $searcher = Search::Kino03::KinoSearch::Searcher->new( index => $folder );
my $hits = $searcher->hits( query => 'b' );
is( $hits->next, 'b', "single segment, single hit" );

add_to_index(qw( d e f g h ));
add_to_index(qw( i j k l m ));

$searcher = Search::Kino03::KinoSearch::Searcher->new( index => $folder );
$hits = $searcher->hits( query => 'f' );
is( $hits->next, 'f', "multiple segments, single hit" );

my $indexer = Search::Kino03::KinoSearch::Indexer->new(
    index  => $folder,
    schema => $schema,
);
$indexer->delete_by_term( field => 'id', term => $_ ) for qw( b f l );
$indexer->optimize;
$indexer->commit;

$searcher = Search::Kino03::KinoSearch::Searcher->new( index => $folder );
$hits = $searcher->hits( query => 'b' );
is( $hits->next, undef, "doc deleted" );

$hits = $searcher->hits( query => 'c' );
is( $hits->next, 'c', "map around deleted doc" );
