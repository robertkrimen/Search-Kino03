use strict;
use warnings;

package Search::Kino03::KinoSearch::Redacted;
use Exporter;
BEGIN {
    our @ISA       = qw( Exporter );
    our @EXPORT_OK = qw( list );
}

# Return a partial list of KinoSearch classes which were once public but are
# now either deprecated, removed, or moved.

sub list {
    return qw(
        Search::Kino03::KinoSearch::Analysis::LCNormalizer
        Search::Kino03::KinoSearch::Analysis::Token
        Search::Kino03::KinoSearch::Analysis::TokenBatch
        Search::Kino03::KinoSearch::Index::Term
        Search::Kino03::KinoSearch::InvIndex
        Search::Kino03::KinoSearch::InvIndexer
        Search::Kino03::KinoSearch::QueryParser::QueryParser
        Search::Kino03::KinoSearch::Search::BooleanQuery
        Search::Kino03::KinoSearch::Search::QueryFilter
        Search::Kino03::KinoSearch::Search::SearchServer
        Search::Kino03::KinoSearch::Search::SearchClient
    );
}

1;
