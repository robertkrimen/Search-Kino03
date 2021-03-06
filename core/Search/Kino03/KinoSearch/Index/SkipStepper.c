#include <stdio.h>

#include "Search/Kino03/KinoSearch/Util/ToolSet.h"

#include "Search/Kino03/KinoSearch/Index/SkipStepper.h"
#include "Search/Kino03/KinoSearch/Store/Folder.h"
#include "Search/Kino03/KinoSearch/Store/InStream.h"
#include "Search/Kino03/KinoSearch/Store/OutStream.h"

SkipStepper*
SkipStepper_new()
{
    SkipStepper *self = (SkipStepper*)VTable_Make_Obj(&SKIPSTEPPER);

    /* Init. */
    self->doc_id   = 0;
    self->filepos  = 0;

    return self;
}

void
SkipStepper_reset(SkipStepper *self, i32_t doc_id, u64_t filepos)
{
    self->doc_id = doc_id;
    self->filepos = filepos;
}

void
SkipStepper_read_record(SkipStepper *self, InStream *instream)
{
    self->doc_id   += InStream_Read_C32(instream);
    self->filepos  += InStream_Read_C64(instream);
}

CharBuf*
SkipStepper_to_string(SkipStepper *self)
{
    char *ptr = MALLOCATE(60, char);
    size_t len = sprintf(ptr, "skip doc: %u file pointer: %" U64P, 
        self->doc_id, self->filepos);
    return CB_new_steal_from_trusted_str(ptr, len, 60);
}

void
SkipStepper_write_record(SkipStepper *self, OutStream *outstream, 
    i32_t last_doc_id, u64_t last_filepos)
{
    const i32_t delta_doc_id = self->doc_id - last_doc_id;
    const u64_t delta_filepos = self->filepos - last_filepos;

    /* Write delta doc id. */
    OutStream_Write_C32(outstream, delta_doc_id);

    /* Write delta file pointer. */
    OutStream_Write_C64(outstream, (u64_t)delta_filepos);
}

/* Copyright 2007-2009 Marvin Humphrey
 *
 * This program is free software; you can redistribute it and/or modify
 * under the same terms as Perl itself.
 */


