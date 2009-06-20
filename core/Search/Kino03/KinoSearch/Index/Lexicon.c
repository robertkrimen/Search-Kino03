#include <string.h>
#include "Search/Kino03/KinoSearch/Util/ToolSet.h"

#include "Search/Kino03/KinoSearch/Index/Lexicon.h"
#include "Search/Kino03/KinoSearch/Util/I32Array.h"
#include "Search/Kino03/KinoSearch/Util/Err.h"

Lexicon*
Lex_init(Lexicon *self)
{
    ABSTRACT_CLASS_CHECK(self, LEXICON);
    return self;
}

/* Copyright 2006-2009 Marvin Humphrey
 *
 * This program is free software; you can redistribute it and/or modify
 * under the same terms as Perl itself.
 */

