use strict;
use warnings;
use lib 'buildlib';

use Test::More tests => 1362;
use Search::Kino03::KinoSearch::Test;
use Search::Kino03::KSx::Search::MockScorer;
use Search::Kino03::KinoSearch::Test::TestUtils qw( modulo_set doc_ids_from_td_coll );

my $sim = Search::Kino03::KinoSearch::Search::Similarity->new;

for my $interval_a ( reverse 1 .. 17 ) {
    for my $interval_b ( reverse 10 .. 17 ) {
        check_scorer( $interval_a, $interval_b );
        for my $interval_c ( 30, 75 ) {
            check_scorer( $interval_a, $interval_b, $interval_c );
            check_scorer( $interval_c, $interval_b, $interval_a );
        }
    }
}
check_scorer(1000);

sub check_scorer {
    my @intervals     = @_;
    my @doc_id_arrays = map { modulo_set( $_, 100 ) } @intervals;
    my @children      = map {
        Search::Kino03::KSx::Search::MockScorer->new(
            doc_ids => $_,
            scores  => [ (0) x scalar @$_ ],
            )
    } @doc_id_arrays;
    my $and_scorer = Search::Kino03::KinoSearch::Search::ANDScorer->new(
        children   => \@children,
        similarity => $sim,
    );
    my @expected  = intersect(@doc_id_arrays);
    my $collector = Search::Kino03::KinoSearch::Search::HitCollector::SortCollector->new(
        wanted => 1000 );
    $and_scorer->collect( collector => $collector );
    is( $collector->get_total_hits,
        scalar @expected,
        "correct num hits @intervals"
    );
    my ( $by_score, $by_id ) = doc_ids_from_td_coll($collector);
    is_deeply( $by_id, \@expected, "correct doc nums @intervals" );
}

sub intersect {
    my @arrays = @_;
    my @out    = @{ $arrays[0] };
    for my $array (@arrays) {
        my %hash;
        @hash{@$array} = (1) x @$array;
        @out = grep { exists $hash{$_} } @out;
    }
    return @out;
}

# Trigger destruction.
undef $sim;
