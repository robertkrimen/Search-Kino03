use strict;
use warnings;
use lib 'buildlib';

use Test::More tests => 3;
use Search::Kino03::KinoSearch::Test::TestUtils qw( test_analyzer );

my $case_folder = Search::Kino03::KinoSearch::Analysis::CaseFolder->new;

test_analyzer( $case_folder, "caPiTal ofFensE",
    ['capital offense'], 'lc plain text' );
