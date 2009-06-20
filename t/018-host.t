use strict;
use warnings;

use Test::More tests => 8;
use Search::Kino03::KinoSearch::Test;
use Search::Kino03::KinoSearch::Util::ToolSet qw( to_perl to_kino );

my $object = Search::Kino03::KinoSearch::Util::Host->new();
isa_ok( $object, "Search::Kino03::KinoSearch::Util::Host" );

is( $object->_callback,   undef, "void callback" );
is( $object->_callback_f, 5,     "float callback" );
is( $object->_callback_i, 5,     "integer callback" );

my $test_obj = $object->_callback_obj;
isa_ok( $test_obj, "Search::Kino03::KinoSearch::Util::ByteBuf" );

my %complex_data_structure = (
    a => [ 1, 2, 3, { ooga => 'booga' } ],
    b => { foo => 'foofoo', bar => 'barbar' },
);
my $kobj = to_kino( \%complex_data_structure );
isa_ok( $kobj, 'Search::Kino03::KinoSearch::Obj' );
my $transformed = to_perl($kobj);
is_deeply( $transformed, \%complex_data_structure,
    "transform from Perl to Kino data structures and back" );

my $bread_and_butter = Search::Kino03::KinoSearch::Util::Hash->new;
$bread_and_butter->store( 'bread', Search::Kino03::KinoSearch::Util::ByteBuf->new('butter') );
my $salt_and_pepper = Search::Kino03::KinoSearch::Util::Hash->new;
$salt_and_pepper->store( 'salt', Search::Kino03::KinoSearch::Util::ByteBuf->new('pepper') );
$complex_data_structure{c} = $bread_and_butter;
$complex_data_structure{d} = $salt_and_pepper;
$transformed               = to_perl( to_kino( \%complex_data_structure ) );
$complex_data_structure{c} = { bread => 'butter' };
$complex_data_structure{d} = { salt => 'pepper' };
is_deeply( $transformed, \%complex_data_structure,
    "handle mixed data structure correctly" );
