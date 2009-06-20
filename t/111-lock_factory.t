use strict;
use warnings;

use Test::More tests => 6;
use Search::Kino03::KinoSearch::Test;

my $folder = Search::Kino03::KinoSearch::Store::RAMFolder->new;

my $lock_factory = Search::Kino03::KinoSearch::Store::LockFactory->new(
    agent_id => 'me',
    folder   => $folder,
);

my $lock = $lock_factory->make_lock(
    lock_name => 'angie',
    timeout   => 1000,
);
is( ref($lock),           'Search::Kino03::KinoSearch::Store::Lock', "make_lock" );
is( $lock->get_lock_name, "angie",                   "correct lock name" );
is( $lock->get_agent_id,  "me",                      "correct agent id" );

$lock = $lock_factory->make_shared_lock(
    lock_name => 'fred',
    timeout   => 0,
);
is( ref($lock), 'Search::Kino03::KinoSearch::Store::SharedLock', "make_shared_lock" );
is( $lock->get_lock_name, "fred", "correct lock name" );
is( $lock->get_agent_id,  "me",   "correct agent id" );
