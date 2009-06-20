use strict;
use warnings;

use Test::More;
use File::Find 'find';

my @modules;

my %excluded = map { ( $_ => 1 ) } qw(
    Search::Kino030::KinoSearch::Analysis::LCNormalizer
    Search::Kino030::KinoSearch::Index::Term
    Search::Kino030::KinoSearch::InvIndex
    Search::Kino030::KinoSearch::InvIndexer
    Search::Kino030::KinoSearch::Search::BooleanQuery
    Search::Kino030::KinoSearch::Search::Scorer
    Search::Kino030::KinoSearch::Search::SearchClient
    Search::Kino030::KinoSearch::Search::SearchServer
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
    s/^.*?(KinoSearch|KSx)/$1/;
    s/\.pm$//;
    s/\W+/::/g;
    $_ = "Search::Kino030::$_";
    if ( $excluded{$_} ) {
        eval qq|use $_;|;
        like( $@, qr/removed|replaced/i,
            "Removed module '$_' throws error on load" );
    }
    else {
        use_ok($_);
    }
}
