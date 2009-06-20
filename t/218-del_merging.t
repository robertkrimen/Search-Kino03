use strict;
use warnings;

use Search::Kino03::KinoSearch::Test;

package DelSchema;
use base 'Search::Kino03::KinoSearch::Schema';

sub new {
    my $self = shift->SUPER::new(@_);
    my $type = Search::Kino03::KinoSearch::FieldType::FullTextType->new(
        analyzer => Search::Kino03::KinoSearch::Analysis::Tokenizer->new, );
    $self->spec_field( name => 'foo', type => $type );
    $self->spec_field( name => 'bar', type => $type );
    return $self;
}

package main;

use Test::More tests => 40;

my $folder  = Search::Kino03::KinoSearch::Store::RAMFolder->new;
my $schema  = DelSchema->new;
my $indexer = Search::Kino03::KinoSearch::Indexer->new(
    index  => $folder,
    schema => $schema,
);
$indexer->add_doc( { foo => 'a', bar => $_ } ) for qw( x y z );
$indexer->commit;

for my $iter ( 1 .. 10 ) {
    is( search_doc('a'), 3, "match all docs prior to deletion $iter" );
    is( search_doc('x'), 1, "match doc to be deleted $iter" );

    $indexer = Search::Kino03::KinoSearch::Indexer->new(
        schema => $schema,
        index  => $folder,
    );
    $indexer->delete_by_term( field => 'bar', term => 'x' );
    $indexer->optimize;
    $indexer->commit;

    is( search_doc('x'), 0, "deletion successful $iter" );
    is( search_doc('a'), 2, "match all docs after deletion $iter" );

    $indexer = Search::Kino03::KinoSearch::Indexer->new(
        schema => $schema,
        index  => $folder,
    );
    $indexer->add_doc( { foo => 'a', bar => 'x' } );
    $indexer->optimize;
    $indexer->commit;
}

sub search_doc {
    my $query_string = shift;
    my $searcher     = Search::Kino03::KinoSearch::Searcher->new( index => $folder );
    my $hits         = $searcher->hits( query => $query_string );
    return $hits->total_hits;
}
