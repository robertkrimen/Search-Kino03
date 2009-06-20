use inc::Module::Install;

name     'Search-Kino030';
all_from 'lib/Search/Kino030.pm';
author   'Robert Krimen <rkrimen@cpan.org>';
license  'perl';

build_requires 'Test::More';

requires 'KinoSearch' => '0.30_01';

auto_install;

WriteAll;

