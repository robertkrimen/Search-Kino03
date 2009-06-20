use strict;
use warnings;
use lib 'buildlib';

use Test::More tests => 5;
use Storable qw( freeze thaw );
use Search::Kino03::KinoSearch::Test::TestUtils qw( create_index );

my $folder = create_index( 'a' .. 'z' );
my $searcher = Search::Kino03::KinoSearch::Searcher->new( index => $folder );

my $no_match_query = Search::Kino03::KinoSearch::Search::NoMatchQuery->new;
is( $no_match_query->to_string, "[NOMATCH]", "to_string" );

my $hits = $searcher->hits( query => $no_match_query );
is( $hits->total_hits, 0, "no matches" );

my $frozen = freeze($no_match_query);
my $thawed = thaw($frozen);
ok( $no_match_query->equals($thawed), "equals" );
$thawed->set_boost(10);
ok( !$no_match_query->equals($thawed), '!equals (boost)' );

my $compiler = $no_match_query->make_compiler( searchable => $searcher );
$frozen = freeze($compiler);
$thawed = thaw($frozen);
ok( $compiler->equals($thawed), "freeze/thaw compiler" );
