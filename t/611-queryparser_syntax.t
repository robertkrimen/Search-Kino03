use strict;
use warnings;

use lib 'buildlib';
use Search::Kino03::KinoSearch::Test;

package MySchema;
use base qw( Search::Kino03::KinoSearch::Schema );

sub new {
    my $self = shift->SUPER::new(@_);
    my $tokenizer = Search::Kino03::KinoSearch::Analysis::Tokenizer->new( pattern => '\S+' );
    my $wordchar_tokenizer
        = Search::Kino03::KinoSearch::Analysis::Tokenizer->new( pattern => '\w+', );
    my $stopalizer
        = Search::Kino03::KinoSearch::Analysis::Stopalizer->new( stoplist => { x => 1 } );
    my $fancy_analyzer = Search::Kino03::KinoSearch::Analysis::PolyAnalyzer->new(
        analyzers => [ $wordchar_tokenizer, $stopalizer, ], );

    my $plain
        = Search::Kino03::KinoSearch::FieldType::FullTextType->new( analyzer => $tokenizer );
    my $fancy = Search::Kino03::KinoSearch::FieldType::FullTextType->new(
        analyzer => $fancy_analyzer );
    $self->spec_field( name => 'plain', type => $plain );
    $self->spec_field( name => 'fancy', type => $fancy );
    return $self;
}

package main;

# Build index.
my $doc_set = Search::Kino03::KinoSearch::Test::TestUtils::doc_set()->to_perl;
my $folder  = Search::Kino03::KinoSearch::Store::RAMFolder->new;
my $schema  = MySchema->new;
my $indexer = Search::Kino03::KinoSearch::Indexer->new(
    schema => $schema,
    index  => $folder,
);
for my $content_string (@$doc_set) {
    $indexer->add_doc(
        {   plain => $content_string,
            fancy => $content_string,
        }
    );
}
$indexer->commit;

Search::Kino03::KinoSearch::Test::TestQueryParserSyntax::run_tests($folder);

