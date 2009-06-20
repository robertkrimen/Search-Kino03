use strict;
use warnings;
use lib 'buildlib';

package MatchSchema::MatchOnly;
use base qw( Search::Kino03::KinoSearch::FieldType::FullTextType );
use Search::Kino03::KinoSearch::Posting::MatchPosting;

sub make_posting {
    if ( @_ == 2 ) {
        return Search::Kino03::KinoSearch::Posting::MatchPosting->new( similarity => $_[1] );
    }
    else {
        shift;
        return Search::Kino03::KinoSearch::Posting::MatchPosting->new(@_);
    }
}

package MatchSchema;
use base qw( Search::Kino03::KinoSearch::Schema );
use Search::Kino03::KinoSearch::Analysis::Tokenizer;

sub new {
    my $self = shift->SUPER::new(@_);
    my $type = MatchSchema::MatchOnly->new(
        analyzer => Search::Kino03::KinoSearch::Analysis::Tokenizer->new );
    $self->spec_field( name => 'content', type => $type );
    return $self;
}

package main;

use Search::Kino03::KinoSearch::Test::TestUtils qw( get_uscon_docs );
use Search::Kino03::KinoSearch::Test::TestSchema;
use Test::More tests => 6;

my $uscon_docs = get_uscon_docs();
my $match_folder = make_index( MatchSchema->new, $uscon_docs );
my $score_folder
    = make_index( Search::Kino03::KinoSearch::Test::TestSchema->new, $uscon_docs );

my $match_searcher = Search::Kino03::KinoSearch::Searcher->new( index => $match_folder );
my $score_searcher = Search::Kino03::KinoSearch::Searcher->new( index => $score_folder );

for (qw( land of the free )) {
    my $match_got = hit_ids_array( $match_searcher, $_ );
    my $score_got = hit_ids_array( $score_searcher, $_ );
    is_deeply( $match_got, $score_got, "same hits for '$_'" );
}

my $qstring          = '"the legislature"';
my $should_have_hits = hit_ids_array( $score_searcher, $qstring );
my $should_be_empty  = hit_ids_array( $match_searcher, $qstring );
ok( scalar @$should_have_hits, "successfully scored phrase $qstring" );
ok( !scalar @$should_be_empty, "no hits matched for phrase $qstring" );

sub make_index {
    my ( $schema, $docs ) = @_;
    my $folder  = Search::Kino03::KinoSearch::Store::RAMFolder->new;
    my $indexer = Search::Kino03::KinoSearch::Indexer->new(
        schema => $schema,
        index  => $folder,
    );
    $indexer->add_doc( { content => $_->{bodytext} } ) for values %$docs;
    $indexer->commit;
    return $folder;
}

sub hit_ids_array {
    my ( $searcher, $query_string ) = @_;
    my $query = $searcher->glean_query($query_string);

    my $bit_vec = Search::Kino03::KinoSearch::Util::BitVector->new(
        capacity => $searcher->doc_max + 1 );
    my $bit_collector = Search::Kino03::KinoSearch::Search::HitCollector::BitCollector->new(
        bit_vector => $bit_vec, );
    $searcher->collect( query => $query, collector => $bit_collector );
    return $bit_vec->to_array->to_arrayref;
}

