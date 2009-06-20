use strict;
use warnings;

use Test::More;
use File::Find 'find';

my @modules;

my %excluded = map { ( $_ => 1 ) } qw(
    Search::Kino03::KinoSearch::Analysis::LCNormalizer
    Search::Kino03::KinoSearch::Index::Term
    Search::Kino03::KinoSearch::InvIndex
    Search::Kino03::KinoSearch::InvIndexer
    Search::Kino03::KinoSearch::Search::BooleanQuery
    Search::Kino03::KinoSearch::Search::Scorer
    Search::Kino03::KinoSearch::Search::SearchClient
    Search::Kino03::KinoSearch::Search::SearchServer
);

find(
    {   no_chdir => 1,
        wanted   => sub {
            return unless $File::Find::name =~ /\.pm$/;
            push @modules, $File::Find::name;
            }
    },
    'lib'
);

plan( tests => scalar @modules );

for (@modules) {
    s#^.*?(Search/Kino03)#$1#;
    s/\.pm$//;
    s/\W+/::/g;
    if ( $excluded{$_} ) {
        eval qq|use $_|;
        like( $@, qr/removed|replaced/i,
            "Removed module '$_' throws error on load" );
    }
    else {
        use_ok($_);
    }
}
