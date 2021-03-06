use strict;
use warnings;

use Test::More tests => 21;
use List::Util qw( shuffle );
use Storable qw( nfreeze thaw );
use Search::Kino03::KinoSearch::Test;
use Search::Kino03::KinoSearch::Util::StringHelper qw( utf8ify );

my ( $hash, @orig, @got );
my ( %source, $dest_ref );

sub build_perl_hashref {
    my $kino_hash = shift;
    my %perl_hash;
    $kino_hash->iter_init;
    while ( my ( $k, $v ) = $kino_hash->iter_next ) {
        $perl_hash{ $k->to_perl } = $v->to_perl;
    }
    return \%perl_hash;
}

sub get_cb { Search::Kino03::KinoSearch::Util::CharBuf->new(shift) }

$hash = Search::Kino03::KinoSearch::Util::Hash->new( capacity => 1 );

@orig = 1 .. 100;
$hash->store( $_, get_cb($_) ) for @orig;
$source{$_} = $_ for @orig;
push @got, $hash->fetch($_) for 1 .. 100;
@got = map { $_->to_string } @got;
is_deeply( \@got, \@orig, "Basic store and fetch" );

is( $hash->get_size, 100, "size incremented properly" );

$hash->store( "40", get_cb("new value") );
is( $hash->fetch("40")->to_string,
    "new value", "store obliterates existing value" );
is( $hash->get_size, 100, "size unaffected after replacement of a val" );
$source{40} = "new value";
$dest_ref = build_perl_hashref($hash);
is_deeply( $dest_ref, \%source, "overwriting doesn't affect other members" );

# Delete.
ok( $hash->delete("40"), "delete returns true when key exists" );
is( $hash->get_size, 99, "delete decrements size when successful" );
$dest_ref = build_perl_hashref($hash);
delete $source{40};
is_deeply( $dest_ref, \%source,
    "successful deleting doesn't affect other members" );

ok( !$hash->delete("40"), "delete returns false when key doesn't exist" );
is( $hash->get_size, 99, "delete doesn't decrement size when unsuccessful" );
$dest_ref = build_perl_hashref($hash);
is_deeply( $dest_ref, \%source,
    "unsuccessful deleting doesn't affect other members" );

# Clear.
$hash->clear;
is( $hash->fetch("2"), undef, "clear" );
is( $hash->get_size,   0,     "size is 0 after clear" );

# iter_init, iter_next.
%source = ();
$hash = Search::Kino03::KinoSearch::Util::Hash->new( capacity => 1 );
for ( 1 .. 10 ) {
    $source{$_} = $_;
    $hash->store( $_, get_cb($_) );
}
$dest_ref = build_perl_hashref($hash);
is_deeply( \%source, $dest_ref, "iteration" );

# Stress test.
%source = ();
$hash = Search::Kino03::KinoSearch::Util::Hash->new( capacity => 1 );
for my $iter ( 1 .. 500 ) {
    my $string = '';
    for my $string_len ( 0 .. int( rand(1200) ) ) {
        $string .= pack( 'C', int( rand(256) ) );
    }
    utf8ify($string);
    $source{$string} = "value: $string";
}
while ( my ( $k, $v ) = each %source ) {
    $hash->store( $k, get_cb($v) );
}
while ( my ( $k, $v ) = each %source ) {
    # Overwrite every pair for good measure.
    $hash->store( $k, get_cb($v) );
}

$dest_ref = build_perl_hashref($hash);
is_deeply( \%source, $dest_ref, "random strings" );

$hash = Search::Kino03::KinoSearch::Util::Hash->new( capacity => 10 );
$hash->store( "foo", get_cb("bar") );
my $foo      = get_cb("foo");
my $key_copy = $hash->find_key(
    key       => $foo,
    hash_code => $foo->hash_code
);
is( $key_copy, "foo", "find_key finds existing key" );
my $bar = get_cb("bar");
$key_copy = $hash->find_key(
    key       => $bar,
    hash_code => $bar->hash_code,
);
is( $key_copy, undef, "find_key doesn't find non-existent key" );

$hash = Search::Kino03::KinoSearch::Util::Hash->new( capacity => 10 );
$hash->store( "foo", get_cb("bar") );
$hash->store( "baz", get_cb("banana") );
my @keys = sort @{ $hash->keys->to_perl };
is_deeply( \@keys, [qw( baz foo )], "keys" );
my @values = sort @{ $hash->values->to_perl };
is_deeply( \@values, [qw( banana bar )], "values" );

my $frozen = nfreeze($hash);
my $thawed = thaw($frozen);
is_deeply( $thawed->to_perl, $hash->to_perl, "freeze/thaw" );

my $ram_file  = Search::Kino03::KinoSearch::Store::RAMFileDes->new;
my $outstream = Search::Kino03::KinoSearch::Store::OutStream->new($ram_file);
$hash->serialize($outstream);
$outstream->close;
my $instream     = Search::Kino03::KinoSearch::Store::InStream->new($ram_file);
my $deserialized = $hash->deserialize($instream);
is_deeply( $hash->to_perl, $deserialized->to_perl, "serialize/deserialize" );
