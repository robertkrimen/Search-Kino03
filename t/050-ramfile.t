use strict;
use warnings;

use Test::More tests => 8;
use Search::Kino03::KinoSearch::Test;

my ( $file_des, $outstream, $instream, $foo );

$file_des = Search::Kino03::KinoSearch::Store::RAMFileDes->new( path => "fake_file" );
$outstream = Search::Kino03::KinoSearch::Store::OutStream->new($file_des);
$outstream->print("foo");
$outstream->flush;
is( $file_des->contents, "foo", '$ramfile->contents' );

my $long_string = 'a' x 5000;
$outstream->print($long_string);
$outstream->flush;

is( $file_des->contents, "foo$long_string",
    "store a string spread out over several buffers" );

$instream = Search::Kino03::KinoSearch::Store::InStream->new($file_des);
$instream->read_bytes( $foo, 3 );
is( $foo, 'foo', "instream reads ramfile properly" );

my $long_dupe;
$instream->read_bytes( $long_dupe, 5000 );
is( $long_dupe, $long_string, "read string spread out over several buffers" );

eval { my $blah; $instream->read_bytes( $blah, 3 ); };
like( $@, qr/EOF/, "reading past EOF throws an error" );

$file_des = Search::Kino03::KinoSearch::Store::RAMFileDes->new( path => "another_fake_file" );
$outstream = Search::Kino03::KinoSearch::Store::OutStream->new($file_des);
my $BUF_SIZE  = Search::Kino03::KinoSearch::Store::FileDes::_BUF_SIZE();
my $rep_count = $BUF_SIZE - 1;
$outstream->print( 'a' x $rep_count );
$outstream->print('foo');
$outstream->close;
$instream = Search::Kino03::KinoSearch::Store::InStream->new($file_des);
$instream->read_bytes( $long_dupe, $rep_count );
undef $foo;
$instream->read_bytes( $foo, 3 );
is( $foo, 'foo', "read across buffer boundary " );

$file_des = Search::Kino03::KinoSearch::Store::RAMFileDes->new( path => "fake_files_galore" );
$outstream = Search::Kino03::KinoSearch::Store::OutStream->new($file_des);
$outstream->print( 'a' x 1024 );
$outstream->print('foo');
$outstream->close;

$instream = Search::Kino03::KinoSearch::Store::InStream->new($file_des);
$instream->seek(1024);
undef $foo;
$instream->read_bytes( $foo, 3 );
is( $foo, 'foo', "InStream seek" );

my $dupe = $instream->reopen( 'foo', 1023, 4 );
undef $foo;
$dupe->read_bytes( $foo, 4 );
is( $foo, 'afoo', "reopened instream" );

