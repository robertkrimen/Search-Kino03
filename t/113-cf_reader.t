use strict;
use warnings;
use lib 'buildlib';

use Test::More tests => 4;
use JSON::XS;
use Search::Kino03::KinoSearch::Test::TestUtils qw( init_test_index_loc );

my $index_loc = init_test_index_loc();
my $folder = Search::Kino03::KinoSearch::Store::FSFolder->new( path => $index_loc );

{
    my $indexer = Search::Kino03::KinoSearch::Indexer->new(
        index  => $folder,
        create => 1,
        schema => Search::Kino03::KinoSearch::Test::TestSchema->new,
    );
    $indexer->add_doc( { content => 'a' } );
    $indexer->commit;
}

my $cf_reader = Search::Kino03::KinoSearch::Store::CompoundFileReader->new(
    folder   => $folder,
    seg_name => 'seg_1',
);

my $instream = $cf_reader->open_in('seg_1/lexicon-1.dat');
isa_ok( $instream, 'Search::Kino03::KinoSearch::Store::InStream' );

my $lex_bytecount = $instream->length;
my $lex_content;
$instream->read_bytes( $lex_content, $lex_bytecount );
my $slurped = $cf_reader->slurp_file('seg_1/lexicon-1.dat');
is( $slurped, $lex_content, "slurp_file gets the right bytes" );

my $alt_lex_content = $folder->slurp_file('seg_1/lexicon-1.dat');
is( $alt_lex_content, $lex_content, "access file content via folder" );

my @files = sort map {"seg_1/$_"} qw(
    documents.ix
    documents.dat
    highlight.ix
    highlight.dat
    postings.skip
    postings-1.dat
    lexicon-1.ixix
    lexicon-1.ix
    lexicon-1.dat
);

my $json        = $folder->slurp_file('seg_1/cfmeta.json');
my $cf_metadata = JSON::XS->new->decode($json);

my @cf_entries = sort keys %{ $cf_metadata->{files} };

is_deeply( \@cf_entries, \@files, "get all the right files" );
