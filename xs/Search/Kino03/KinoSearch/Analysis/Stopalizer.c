#include "Search/Kino03/KinoSearch/Util/ToolSet.h"

#include "Search/Kino03/KinoSearch/Analysis/Stopalizer.h"
#include "Search/Kino03/KinoSearch/Util/Host.h"

Hash*
Stopalizer_gen_stoplist(const CharBuf *language)
{
    return (Hash*)Host_callback_obj(&STOPALIZER, "gen_stoplist", 1,
        ARG_STR("language", language));
}

/* Copyright 2005-2009 Marvin Humphrey
 *
 * This program is free software; you can redistribute it and/or modify
 * under the same terms as Perl itself.
 */

