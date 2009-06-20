use strict;
use warnings;
use utf8;

use Test::More tests => 5;
use Search::Kino03::KinoSearch::Test;

package TestAnalyzer;
use base qw( Search::Kino03::KinoSearch::Analysis::Analyzer );
sub transform { $_[1] }

package MySchema;
use base qw( Search::Kino03::KinoSearch::Schema );

sub new {
    my $self = shift->SUPER::new(@_);
    my $type = Search::Kino03::KinoSearch::FieldType::FullTextType->new(
        analyzer => TestAnalyzer->new );
    $self->spec_field( name => 'a', type => $type );
    $self->spec_field( name => 'b', type => $type );
    $self->spec_field( name => 'c', type => $type );
    return $self;
}

package main;

my $folder  = Search::Kino03::KinoSearch::Store::RAMFolder->new;
my $schema  = MySchema->new;
my $indexer = Search::Kino03::KinoSearch::Indexer->new(
    create => 1,
    index  => $folder,
    schema => $schema,
);

# We need to test strings that exceed the Latin-1 range to make sure that
# get_term treats them correctly. (See change 3103 in the svn repo.)
my @animals = qw( cat dog sloth λεοντάρι змейка );
for my $animal (@animals) {
    $indexer->add_doc(
        {   a => $animal,
            b => $animal,
            c => $animal,
        }
    );
}
$indexer->commit;

my $snapshot
    = Search::Kino03::KinoSearch::Index::Snapshot->new->read_file( folder => $folder );
my $segment = Search::Kino03::KinoSearch::Index::Segment->new(
    folder => $folder,
    name   => 'seg_1',
);
$segment->read_file;
my $lex_reader = Search::Kino03::KinoSearch::Index::DefaultLexiconReader->new(
    schema   => $schema,
    folder   => $folder,
    snapshot => $snapshot,
    segments => [$segment],
    seg_tick => 0,
);
my %lexicons;
for (qw( a b c )) {
    $lexicons{$_} = $lex_reader->lexicon( field => $_ );
}

my @fields;
my @terms;
for (qw( a b c )) {
    my $lexicon = $lexicons{$_};
    while ( $lexicon->next ) {
        push @fields, $lexicon->get_field;
        push @terms,  $lexicon->get_term;
    }
}
is_deeply( \@fields, [qw( a a a a a b b b b b c c c c c )],
    "correct fields" );
my @correct_texts = (@animals) x 3;
is_deeply( \@terms, \@correct_texts, "correct terms" );

my $lexicon = $lexicons{b};
$lexicon->seek("dog");
$lexicon->next;
is( $lexicon->get_term,  'sloth', "lexicon seeks to correct term (ptr)" );
is( $lexicon->get_field, 'b',     "lexicon has correct field" );

$lexicon->reset;
$lexicon->next;
is( $lexicon->get_term, 'cat', "reset" );
