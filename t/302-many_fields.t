use strict;
use warnings;

package MySchema;
use base qw( Search::Kino03::KinoSearch::Schema );
use Search::Kino03::KinoSearch::Analysis::Tokenizer;

our %fields = ();

package main;

use Test::More tests => 10;
use Search::Kino03::KinoSearch::Test;

my $schema = MySchema->new;
my $type   = Search::Kino03::KinoSearch::FieldType::FullTextType->new(
    analyzer => Search::Kino03::KinoSearch::Analysis::Tokenizer->new, );

for my $num_fields ( 1 .. 10 ) {
    # Build an index with $num_fields fields, and the same content in each.
    $schema->spec_field( name => "field$num_fields", type => $type );
    my $folder  = Search::Kino03::KinoSearch::Store::RAMFolder->new;
    my $indexer = Search::Kino03::KinoSearch::Indexer->new(
        schema => $schema,
        index  => $folder,
    );

    for my $content ( 'a' .. 'z', 'x x y' ) {
        my %doc;
        for ( 1 .. $num_fields ) {
            $doc{"field$_"} = $content;
        }
        $indexer->add_doc( \%doc );
    }
    $indexer->commit;

    # See if our search results match as expected.
    my $searcher = Search::Kino03::KinoSearch::Searcher->new( index => $folder );
    my $hits = $searcher->hits(
        query      => 'x',
        num_wanted => 100,
    );
    is( $hits->total_hits, 2,
        "correct number of hits for $num_fields fields" );
    my $top_hit = $hits->next;
}
