use strict;
use warnings;

use Search::Kino03::KinoSearch;

1;

__END__

__XS__

MODULE = Search::Kino03::KinoSearch   PACKAGE = Search::Kino03::KinoSearch::Util::Debug

void
DEBUG_PRINT(message)
    char *message;
PPCODE:
    KINO_DEBUG_PRINT("%s", message);

void
DEBUG(message)
    char *message;
PPCODE:
    KINO_DEBUG("%s", message);

chy_bool_t
DEBUG_ENABLED()
CODE:
    RETVAL = KINO_DEBUG_ENABLED;
OUTPUT: RETVAL

=for comment

Keep track of any KS objects that have been assigned to global Perl variables.
This is useful when accounting how many objects should have been destroyed and
diagnosing memory leaks.

=cut

void
track_globals(...)
PPCODE:
{
    CHY_UNUSED_VAR(items);
    KINO_IFDEF_DEBUG(kino_Debug_num_globals++;);
}

void
set_env_cache(str)
    char *str;
PPCODE:
    kino_Debug_set_env_cache(str);

void
ASSERT(maybe)
    int maybe;
PPCODE:
    KINO_ASSERT(maybe, "XS ASSERT binding test");

IV
num_allocated()
CODE:
    RETVAL = kino_Debug_num_allocated;
OUTPUT: RETVAL

IV
num_freed()
CODE:
    RETVAL = kino_Debug_num_freed;
OUTPUT: RETVAL

IV
num_globals()
CODE:
    RETVAL = kino_Debug_num_globals;
OUTPUT: RETVAL

__COPYRIGHT__

Copyright 2005-2009 Marvin Humphrey

This program is free software; you can redistribute it and/or modify
under the same terms as Perl itself.

