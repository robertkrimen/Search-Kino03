#include "Search/Kino03/KinoSearch/Util/ToolSet.h"

#include "Search/Kino03/KinoSearch/Index/SegReader.h"
#include "Search/Kino03/KinoSearch/Util/Host.h"

CharBuf*
SegReader_try_init_components(SegReader *self)
{
    return (CharBuf*)Host_callback_obj(self, "try_init_components", 0);
}

/* Copyright 2009 Marvin Humphrey
 *
 * This program is free software; you can redistribute it and/or modify
 * under the same terms as Perl itself.
 */

