use strict;
use warnings;
use lib 'buildlib';

package MyTokenizer;
use base qw( Search::Kino03::KinoSearch::Analysis::Analyzer );
use Search::Kino03::KinoSearch::Analysis::Inversion;

sub transform {
    my ( $self, $inversion ) = @_;
    my $new_inversion = Search::Kino03::KinoSearch::Analysis::Inversion->new;

    while ( my $token = $inversion->next ) {
        for ( $token->get_text ) {
            my $this_time = /z/ ? 1 : 0;
            # Accumulate token start_offsets and end_offsets.
            while (/(\w)/g) {
                # Special boost just for one doc.
                my $boost = ( $1 eq 'a' and $this_time ) ? 100 : 1;
                $new_inversion->append(
                    Search::Kino03::KinoSearch::Analysis::Token->new(
                        text         => $1,
                        start_offset => $-[0],
                        end_offset   => $+[0],
                        boost        => $boost,
                    ),
                );
            }
        }
    }

    return $new_inversion;
}

sub equals {
    my ( $self, $other ) = @_;
    return 0 unless ref($self) eq ref($other);
    return 1;
}

package MySchema::boosted;
use base qw( Search::Kino03::KinoSearch::FieldType::FullTextType );
use Search::Kino03::KinoSearch::Posting::RichPosting;

sub make_posting {
    if ( @_ == 2 ) {
        return Search::Kino03::KinoSearch::Posting::RichPosting->new( similarity => $_[1] );
    }
    else {
        shift;
        return Search::Kino03::KinoSearch::Posting::RichPosting->new(@_);
    }
}

package MySchema;
use base qw( Search::Kino03::KinoSearch::Schema );
use Search::Kino03::KinoSearch::Analysis::Tokenizer;

sub new {
    my $self       = shift->SUPER::new(@_);
    my $plain_type = Search::Kino03::KinoSearch::FieldType::FullTextType->new(
        analyzer => Search::Kino03::KinoSearch::Analysis::Tokenizer->new );
    my $boosted_type
        = MySchema::boosted->new( analyzer => MyTokenizer->new, );
    $self->spec_field( name => 'plain',   type => $plain_type );
    $self->spec_field( name => 'boosted', type => $boosted_type );
    return $self;
}

package main;

use Test::More tests => 2;

my $good    = "x x x a a x x x x x x x x";
my $better  = "x x x a a a x x x x x x x";
my $best    = "x x x a a a a a a a a a a";
my $boosted = "z x x a x x x x x x x x x";

my $schema  = MySchema->new;
my $folder  = Search::Kino03::KinoSearch::Store::RAMFolder->new;
my $indexer = Search::Kino03::KinoSearch::Indexer->new(
    schema => $schema,
    index  => $folder,
);

for ( $good, $better, $best, $boosted ) {
    $indexer->add_doc( { plain => $_, boosted => $_ } );
}
$indexer->commit;

my $searcher = Search::Kino03::KinoSearch::Searcher->new( index => $folder );

my $q_for_plain = Search::Kino03::KinoSearch::Search::TermQuery->new(
    field => 'plain',
    term  => 'a',
);
my $hits = $searcher->hits( query => $q_for_plain );
is( $hits->next->{plain},
    $best, "verify that search on unboosted field returns best match" );

my $q_for_boosted = Search::Kino03::KinoSearch::Search::TermQuery->new(
    field => 'boosted',
    term  => 'a',
);
$hits = $searcher->hits( query => $q_for_boosted );
is( $hits->next->{boosted},
    $boosted, "artificially boosted token overrides better match" );

__END__

TODO: {
    local $TODO = "positions not passed to boolscorer correctly yet";
    is_deeply(
        \@contents,
        [ $best, $better, $good ],
        "proximity helps boost scores"
    );
}
