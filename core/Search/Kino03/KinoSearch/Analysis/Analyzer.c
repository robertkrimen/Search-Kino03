#include "Search/Kino03/KinoSearch/Util/ToolSet.h"

#include "Search/Kino03/KinoSearch/Analysis/Analyzer.h"
#include "Search/Kino03/KinoSearch/Analysis/Token.h"
#include "Search/Kino03/KinoSearch/Analysis/Inversion.h"

Analyzer*
Analyzer_init(Analyzer *self)
{
    ABSTRACT_CLASS_CHECK(self, ANALYZER);
    return self;
}

Inversion*
Analyzer_transform_text(Analyzer *self, CharBuf *text)
{
    size_t token_len = CB_Get_Size(text);
    Token *seed = Token_new((char*)CB_Get_Ptr8(text), token_len, 0, token_len, 
        1.0, 1);
    Inversion *starter = Inversion_new(seed);
    Inversion *retval  = Analyzer_Transform(self, starter);
    DECREF(seed);
    DECREF(starter);
    return retval;
}

VArray*
Analyzer_split(Analyzer *self, CharBuf *text)
{
    Inversion  *inversion = Analyzer_Transform_Text(self, text);
    VArray     *out       = VA_new(0);
    Token      *token;

    while ((token = Inversion_Next(inversion)) != NULL) {
        VA_Push(out, (Obj*)CB_new_from_trusted_utf8(token->text, token->len));
    }

    DECREF(inversion);

    return out;
}

bool_t
Analyzer_dump_equals(Analyzer *self, Obj *dump)
{
    Hash *source = (Hash*)dump;
    if (!OBJ_IS_A(dump, HASH)) return false;
    else {
        CharBuf *class_name = (CharBuf*)Hash_Fetch_Str(source, "_class", 6);
        if (!class_name) return false;
        if (!CB_Equals(Obj_Get_Class_Name(self), (Obj*)class_name)) {
            return false;
        }
    }
    return true;
}

/* Copyright 2006-2009 Marvin Humphrey
 *
 * This program is free software; you can redistribute it and/or modify
 * under the same terms as Perl itself.
 */

