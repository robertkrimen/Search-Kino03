#include "xs/XSBind.h"

#include "Search/Kino03/KinoSearch/Analysis/Stemmer.h"
#include "Search/Kino03/KinoSearch/Util/Host.h"

void
kino_Stemmer_load_snowball() 
{
    kino_Host_callback(&KINO_STEMMER, "lazy_load_snowball", 0);
}
    
/* Copyright 2005-2009 Marvin Humphrey
 *
 * This program is free software; you can redistribute it and/or modify
 * under the same terms as Perl itself.
 */

