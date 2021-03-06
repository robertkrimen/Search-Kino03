#include "Search/Kino03/KinoSearch/Util/ToolSet.h"

#include "Search/Kino03/KinoSearch/Doc.h"

Doc*
Doc_new(void *fields, i32_t doc_id)
{
    Doc *self = (Doc*)VTable_Make_Obj(&DOC);
    return Doc_init(self, fields, doc_id);
}

void  
Doc_set_doc_id(Doc *self, i32_t doc_id) { self->doc_id = doc_id; }
i32_t 
Doc_get_doc_id(Doc *self) { return self->doc_id;   }
void* 
Doc_get_fields(Doc *self) { return self->fields;   }

/* Copyright 2006-2009 Marvin Humphrey
 *
 * This program is free software; you can redistribute it and/or modify
 * under the same terms as Perl itself.
 */

