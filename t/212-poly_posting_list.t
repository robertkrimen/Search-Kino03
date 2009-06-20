use strict;
use warnings;

use Search::Kino03::KinoSearch::Test;

package MySchema;
use base qw( Search::Kino03::KinoSearch::Schema );

sub new {
    my $self = shift->SUPER::new(@_);
    my $type = Search::Kino03::KinoSearch::FieldType::FullTextType->new(
        analyzer => Search::Kino03::KinoSearch::Analysis::Tokenizer->new, );
    $self->spec_field( name => 'content', type => $type );
    $self->spec_field( name => 'id',      type => $type );
    return $self;
}

package main;
use Test::More tests => 4;

my $folder = Search::Kino03::KinoSearch::Store::RAMFolder->new();
my $schema = MySchema->new;

my $id = 1;
for my $iter ( 1 .. 4 ) {
    my $indexer = Search::Kino03::KinoSearch::Indexer->new(
        index  => $folder,
        schema => $schema,
    );
    for my $letter ( 'a' .. 'y' ) {
        my $content = ( "$letter " x $iter ) . 'z';
        $indexer->add_doc(
            {   content => $content,
                id      => $id++,
            }
        );
    }
    $indexer->commit;
}

my $reader = Search::Kino03::KinoSearch::Index::IndexReader->open( index => $folder );
my $plist = $reader->fetch("Search::Kino03::KinoSearch::Index::PostingsReader")
    ->posting_list( field => 'content', term => 'c' );

my @doc_ids;
my @doc_ids_from_next;
my @freqs;
my @prox;
while ( my $doc_id_from_next = $plist->next ) {
    push @doc_ids_from_next, $doc_id_from_next;
    my $posting = $plist->get_posting;
    push @doc_ids, $posting->get_doc_id;
    push @freqs,   $posting->get_freq;
    push @prox,    @{ $posting->get_prox };
}
my @correct_doc_ids = ( 3, 28, 53, 78 );
is_deeply( \@doc_ids, \@correct_doc_ids, "correct doc_ids" );
is_deeply( \@doc_ids_from_next, \@correct_doc_ids,
    "next returns correct doc nums" );

is_deeply( \@freqs, [ 1, 2, 3, 4 ], "correct freqs" );
is_deeply( \@prox, [ 0, 0, 1, 0, 1, 2, 0, 1, 2, 3 ], "correct positions" );
