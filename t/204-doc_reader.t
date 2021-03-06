use strict;
use warnings;

use Test::More tests => 4;

package TestAnalyzer;
use base qw( Search::Kino03::KinoSearch::Analysis::Analyzer );
sub transform { $_[1] }

package main;
use Encode qw( _utf8_on );

sub new_schema {
    my $schema   = Search::Kino03::KinoSearch::Schema->new;
    my $analyzer = TestAnalyzer->new;
    my $fulltext
        = Search::Kino03::KinoSearch::FieldType::FullTextType->new( analyzer => $analyzer );
    my $bin        = Search::Kino03::KinoSearch::FieldType::BlobType->new;
    my $not_stored = Search::Kino03::KinoSearch::FieldType::FullTextType->new(
        analyzer => $analyzer,
        stored   => 0,
    );
    $schema->spec_field( name => 'text',     type => $fulltext );
    $schema->spec_field( name => 'bin',      type => $bin );
    $schema->spec_field( name => 'unstored', type => $not_stored );
    $schema->spec_field( name => 'empty',    type => $fulltext );
    return $schema;
}

# This valid UTF-8 string includes skull and crossbones, null byte -- however,
# the binary value is not flagged as UTF-8.
my $bin_val = my $val = "a b c \xe2\x98\xA0 \0a";
_utf8_on($val);

my $folder = Search::Kino03::KinoSearch::Store::RAMFolder->new;
my $schema = new_schema();

my $indexer = Search::Kino03::KinoSearch::Indexer->new(
    index  => $folder,
    schema => $schema,
    create => 1,
);
$indexer->add_doc(
    {   text     => $val,
        bin      => $bin_val,
        unstored => $val,
        empty    => '',
    }
);
$indexer->commit;

my $snapshot
    = Search::Kino03::KinoSearch::Index::Snapshot->new->read_file( folder => $folder );
my $segment = Search::Kino03::KinoSearch::Index::Segment->new(
    folder => $folder,
    name   => 'seg_1'
);
$segment->read_file;
my $doc_reader = Search::Kino03::KinoSearch::Index::DefaultDocReader->new(
    schema   => $schema,
    folder   => $folder,
    snapshot => $snapshot,
    segments => [$segment],
    seg_tick => 0,
);

my $doc = $doc_reader->fetch( doc_id => 0 );

is( $doc->{text},     $val,     "text" );
is( $doc->{bin},      $bin_val, "bin" );
is( $doc->{unstored}, undef,    "unstored" );
is( $doc->{empty},    '',       "empty" );
