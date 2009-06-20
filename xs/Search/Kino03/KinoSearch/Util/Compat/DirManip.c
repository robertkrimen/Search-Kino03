#include <string.h>
#include <stdlib.h>

#include "Search/Kino03/KinoSearch/Util/Compat/DirManip.h"
#include "Search/Kino03/KinoSearch/Util/CharBuf.h"
#include "Search/Kino03/KinoSearch/Util/VArray.h"
#include "Search/Kino03/KinoSearch/Util/Err.h"
#include "Search/Kino03/KinoSearch/Util/Host.h"

#include <sys/stat.h>

kino_CharBuf*
kino_DirManip_absolutify(const kino_CharBuf *path)
{
   
    return kino_Host_callback_str(&KINO_DIRMANIP,
            "absolutify", 1, KINO_ARG_STR("path", path));
}
