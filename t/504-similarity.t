use strict;
use warnings;

package MockSearchable;
use base qw( Search::Kino03::KinoSearch::Search::Searchable );

our %doc_freqs;

sub new {
    my ( $class, %args ) = @_;
    my $doc_freqs = delete $args{doc_freqs};
    my $self      = $class->SUPER::new(%args);
    $doc_freqs{$$self} = $doc_freqs;
    return $self;
}

sub DESTROY {
    my $self = shift;
    delete $doc_freqs{$$self};
    $self->SUPER::DESTROY;
}

sub doc_freq {
    my ( $self, %args ) = @_;
    return $doc_freqs{$$self}{ $args{term} };
}

sub doc_max {100}

package MySchema::LongTextField;
use base qw( Search::Kino03::KinoSearch::FieldType::FullTextType );
use Search::Kino03::KSx::Search::LongFieldSim;

sub make_similarity { Search::Kino03::KSx::Search::LongFieldSim->new }

package MySchema;
use base qw( Search::Kino03::KinoSearch::Schema );
use Search::Kino03::KinoSearch::Analysis::Tokenizer;

sub new {
    my $self     = shift->SUPER::new(@_);
    my $analyzer = Search::Kino03::KinoSearch::Analysis::Tokenizer->new;
    my $plain_type
        = Search::Kino03::KinoSearch::FieldType::FullTextType->new( analyzer => $analyzer, );
    my $long_field_type
        = MySchema::LongTextField->new( analyzer => $analyzer, );
    $self->spec_field( name => 'title', type => $plain_type );
    $self->spec_field( name => 'body',  type => $long_field_type );
    return $self;
}

package main;
use Test::More tests => 9;
use Search::Kino03::KinoSearch::Test;
use bytes;
no bytes;

my $sim       = Search::Kino03::KinoSearch::Search::Similarity->new;
my $evil_twin = $sim->load( $sim->dump );
ok( $sim->equals($evil_twin), "Dump/Load" );

cmp_ok( $sim->tf(10) - $sim->tf(9), '<', 1, "TF is damped" );

my $mock_searchable = MockSearchable->new(
    schema    => MySchema->new,
    doc_freqs => {
        foo => 3,
        bar => 200,
    },
);
my $foo_idf = $sim->idf(
    searchable => $mock_searchable,
    field      => 'title',
    term       => 'foo'
);
my $bar_idf = $sim->idf(
    searchable => $mock_searchable,
    field      => 'title',
    term       => 'bar'
);
cmp_ok( $foo_idf, '>', $bar_idf, 'Rarer terms have higher IDF' );

my $less_coordinated = $sim->coord( overlap => 2, max_overlap => 5 );
my $more_coordinated = $sim->coord( overlap => 3, max_overlap => 5 );
cmp_ok( $less_coordinated, '<', $more_coordinated,
    "greater overlap means bigger coord bonus" );

my @bytes  = ( 100,      110,     120, 130, 140 );
my @floats = ( 0.015625, 0.09375, 0.5, 3.0, 16.0 );
my @transformed = map { $sim->decode_norm($_) } @bytes;
is_deeply( \@floats, \@transformed,
    "decode_norm more or less matches Java Lucene behavior" );

@bytes       = 0 .. 255;
@floats      = map { $sim->decode_norm($_) } @bytes;
@transformed = map { $sim->encode_norm($_) } @floats;
is_deeply( \@transformed, \@bytes,
    "encode_norm and decode_norm are complementary" );

my $norm_decoder = $sim->get_norm_decoder;
@transformed = ();
for ( 0 .. 255 ) {
    push @transformed,
        unpack( 'f', bytes::substr( $norm_decoder, $_ * 4, 4 ) );
}
is_deeply( \@transformed, \@floats,
    "using the norm_decoder produces desired results" );

my $folder  = Search::Kino03::KinoSearch::Store::RAMFolder->new;
my $indexer = Search::Kino03::KinoSearch::Indexer->new(
    index  => $folder,
    schema => MySchema->new,
);

my %source_docs = (
    'spam'     => 'spam spam',
    'not spam' => 'not spam not even close to spam no spam here',
);
while ( my ( $title, $body ) = each %source_docs ) {
    $indexer->add_doc(
        {   title => $title,
            body  => $body,
        }
    );
}
$indexer->commit;
undef $indexer;

my $searcher = Search::Kino03::KinoSearch::Searcher->new( index => $folder );

my $hits = $searcher->hits(
    query => Search::Kino03::KinoSearch::Search::TermQuery->new(
        field => 'title',
        term  => 'spam',
    )
);
is( $hits->next->{'title'},
    'spam', "Default Similarity biased towards short fields" );

$hits = $searcher->hits(
    query => Search::Kino03::KinoSearch::Search::TermQuery->new(
        field => 'body',
        term  => 'spam',
    )
);
is( $hits->next->{'title'},
    'not spam', "LongFieldSim cancels short-field bias" );
