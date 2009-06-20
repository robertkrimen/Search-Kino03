use strict;
use warnings;
use lib 'buildlib';

use Test::More tests => 7;
use Storable qw( freeze thaw );
use Search::Kino03::KinoSearch::Test;

my $match_doc = Search::Kino03::KinoSearch::Search::MatchDoc->new(
    doc_id => 31,
    score  => 5.0,
);
is( $match_doc->get_doc_id, 31,    "get_doc_id" );
is( $match_doc->get_score,  5.0,   "get_score" );
is( $match_doc->get_values, undef, "get_values" );
my $match_doc_copy = thaw( freeze($match_doc) );
is( $match_doc_copy->get_doc_id, $match_doc->get_doc_id,
    "doc_id survives serialization" );
is( $match_doc_copy->get_score, $match_doc->get_score,
    "score survives serialization" );
is( $match_doc_copy->get_values, $match_doc->get_values,
    "empty values still empty after serialization" );

my $values = Search::Kino03::KinoSearch::Util::VArray->new( capacity => 4 );
$values->store( 0, Search::Kino03::KinoSearch::Util::CharBuf->new("foo") );
$values->store( 3, Search::Kino03::KinoSearch::Util::CharBuf->new("bar") );
$match_doc = Search::Kino03::KinoSearch::Search::MatchDoc->new(
    doc_id => 120,
    score  => 35,
    values => $values,
);
$match_doc_copy = thaw( freeze($match_doc) );
is_deeply(
    $match_doc_copy->get_values,
    [ 'foo', undef, undef, 'bar' ],
    "values array survives serialization"
);

