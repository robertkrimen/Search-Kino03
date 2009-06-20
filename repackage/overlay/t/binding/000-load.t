use strict;
use warnings;

use Test::More;
use File::Find 'find';

my @modules;

my %excluded = map { ( $_ => 1 ) } qw(
    ___KS_PACKAGE___::KinoSearch::Analysis::LCNormalizer
    ___KS_PACKAGE___::KinoSearch::Index::Term
    ___KS_PACKAGE___::KinoSearch::InvIndex
    ___KS_PACKAGE___::KinoSearch::InvIndexer
    ___KS_PACKAGE___::KinoSearch::Search::BooleanQuery
    ___KS_PACKAGE___::KinoSearch::Search::Scorer
    ___KS_PACKAGE___::KinoSearch::Search::SearchClient
    ___KS_PACKAGE___::KinoSearch::Search::SearchServer
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
    s#^.*?(___KS_PACKAGE_PATH___)#$1#;
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
