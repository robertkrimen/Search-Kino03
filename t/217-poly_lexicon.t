use strict;
use warnings;

use lib 'buildlib';
use Search::Kino03::KinoSearch::Test;

use strict;
use warnings;

package MyArchitecture;
use base qw( Search::Kino03::KinoSearch::Architecture );

sub index_interval { confess("should be displaced via local() below") }

package MySchema;
use base qw( Search::Kino03::KinoSearch::Schema );

sub new {
    my $self = shift->SUPER::new(@_);
    my $type = Search::Kino03::KinoSearch::FieldType::StringType->new( sortable => 1 );
    $self->spec_field( name => 'content', type => $type );
    return $self;
}

sub architecture { MyArchitecture->new }

package main;
use Test::More tests => 35;
use File::Spec::Functions qw( catfile );

my @docs = ( ( ("a") x 100 ), 'j', "Foo" );
my @chars = ( 'a' .. 'z' );
for ( 0 .. 100 ) {
    my $content = '';
    for my $num_chars ( 1 .. int( rand(10) + 1 ) ) {
        $content .= @chars[ rand(@chars) ];
    }
    push @docs, "$content";
}

# Accumulate unique sorted terms.
my @correct = ();
for ( sort @docs ) {
    next if ( @correct and $_ eq $correct[-1] );
    push @correct, $_;
}

# Remember where 'j' exists in our 'correct' list.
my $correct_term_num = 0;
for (@correct) {
    last if $correct[$correct_term_num] eq 'j';
    $correct_term_num++;
}

for my $index_interval ( 1, 2, 3, 4, 7, 128, 1024 ) {

    no warnings 'redefine';
    local *MyArchitecture::index_interval = sub {$index_interval};

    my $folder = Search::Kino03::KinoSearch::Store::RAMFolder->new;
    my $schema = MySchema->new;

    my @docs_copy = @docs;
    while (@docs_copy) {
        my $indexer = Search::Kino03::KinoSearch::Indexer->new(
            index  => $folder,
            schema => $schema,
        );
        my $docs_this_seg = int( rand(@docs_copy) );
        $docs_this_seg = 10 if $docs_this_seg < 10;

        for ( 0 .. 10 ) {
            last unless @docs_copy;
            my $tick = int( rand(@docs_copy) );
            $indexer->add_doc(
                { content => splice( @docs_copy, $tick, 1 ) } );
        }
        $indexer->commit;
    }

    my $reader = Search::Kino03::KinoSearch::Index::IndexReader->open( index => $folder, );

    my $lexicon = $reader->obtain("Search::Kino03::KinoSearch::Index::LexiconReader")
        ->lexicon( field => 'content' );
    isa_ok( $lexicon, "Search::Kino03::KinoSearch::Index::PolyLexicon" );

    $lexicon->next;
    is( $lexicon->get_term, $correct[0],
        "calling lexicon() without term returns Lexicon with iterator reset"
    );
    $lexicon->reset;

    my @got;
    while ( $lexicon->next ) {
        push @got, $lexicon->get_term;
    }

    is_deeply( \@got, \@correct,
        "correct order for random strings (interval: $index_interval)" );

    $lexicon->seek('j');
    is( $lexicon->get_term, 'j', "seek (interval: $index_interval)" );

    $lexicon->seek(undef);
    ok( !defined $lexicon->get_term, "seek to undef resets" );
}
