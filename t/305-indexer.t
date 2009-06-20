use strict;
use warnings;
use lib 'buildlib';

use Test::More tests => 3;
use Search::Kino03::KinoSearch::Test::TestSchema;

my $folder = Search::Kino03::KinoSearch::Store::RAMFolder->new;
my $schema = Search::Kino03::KinoSearch::Test::TestSchema->new;

my $indexer = Search::Kino03::KinoSearch::Indexer->new(
    index  => $folder,
    schema => $schema,
);

$indexer->add_doc( { content => 'foo' } );
undef $indexer;

$indexer = Search::Kino03::KinoSearch::Indexer->new(
    index  => $folder,
    schema => $schema,
);
$indexer->add_doc( { content => 'foo' } );
pass("Indexer ignores garbage from interrupted session");

SKIP: {
    skip( 1, "Known leak, though might be fixable" ) if $ENV{KINO_VALGRIND};
    eval {
        my $lock_factory = Search::Kino03::KinoSearch::Store::LockFactory->new(
            folder   => $folder,
            agent_id => 'somebody_else',
        );
        my $inv = Search::Kino03::KinoSearch::Indexer->new(
            lock_factory => $lock_factory,
            index        => $folder,
            schema       => $schema,
        );
    };
    like( $@, qr/write\.lock/, "failed to get lock with competing host" );
}

my $pid = 12345678;
do {
    # Fake a write lock.
    $folder->delete("write.lock") or die "Couldn't delete 'write.lock'";
    my $outstream = $folder->open_out('write.lock')
        or die "Can't open write.lock";
    while ( kill( 0, $pid ) ) {
        $pid++;
    }
    $outstream->print(
        qq|
        {  
            "agent_id": "somebody_else",
            "pid": $pid,
            "lock_name": "write"
        }|
    );
    $outstream->close;

    eval {
        my $lock_factory = Search::Kino03::KinoSearch::Store::LockFactory->new(
            agent_id => 'somebody_else',
            folder   => $folder,
        );
        my $inv = Search::Kino03::KinoSearch::Indexer->new(
            lock_factory => $lock_factory,
            schema       => $schema,
            index        => $folder,
        );
    };

    # Mitigate (though not eliminate) race condition false failure.
} while ( kill( 0, $pid ) );

ok( !$@, "clobbered lock from same host with inactive pid" );
