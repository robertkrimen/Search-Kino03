use 5.008003;
use strict;
use warnings;

my $conflict = 0;
my $existing;

use lib 'buildlib';
use KinoSearch::Build;
use File::Spec::Functions qw( curdir );

my $builder = KinoSearch::Build->new(
    module_name       => 'Search::Kino03',
    license           => 'perl',
    dist_author       => 'Robert Krimen <rkrimen@cpan.org>',
    dist_version_from => 'lib/Search/Kino03.pm',
    requires          => {
        'Lingua::Stem::Snowball' => 0.952,
        'Lingua::StopWords'      => 0.09,
        'JSON::XS'               => 1.53,
    },
    build_requires => {
        'ExtUtils::CBuilder' => 0,
        'ExtUtils::ParseXS'  => 0,
        'Parse::RecDescent'  => 0,
    },
    create_makefile_pl => 'passthrough',
    add_to_cleanup =>
        [ 'MANIFEST.bak', 'perltidy.ERR', '*.pdb', '*.manifest' ],
    meta_merge => {
        resources => {
            repository => 'http://github.com/robertkrimen/Search-Kino03/tree/master',
        },
    },
);

if (-e 'MANIFEST.SKIP') {
    my $all_from = join '/', 'lib', split m/::/, $builder->module_name . '.pm';
    `perldoc -tF $all_from > README` if ! -e 'README' || (stat $all_from)[9] > (stat 'README')[9];
}

$builder->create_build_script();
