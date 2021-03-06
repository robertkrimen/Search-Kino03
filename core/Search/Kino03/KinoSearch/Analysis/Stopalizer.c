#include "Search/Kino03/KinoSearch/Util/ToolSet.h"

#include "Search/Kino03/KinoSearch/Analysis/Stopalizer.h"
#include "Search/Kino03/KinoSearch/Analysis/Token.h"
#include "Search/Kino03/KinoSearch/Analysis/Inversion.h"

Stopalizer*
Stopalizer_new(const CharBuf *language, Hash *stoplist)
{
    Stopalizer *self = (Stopalizer*)VTable_Make_Obj(&STOPALIZER);
    return Stopalizer_init(self, language, stoplist);
}

Stopalizer*
Stopalizer_init(Stopalizer *self, const CharBuf *language, Hash *stoplist)
{
    Analyzer_init((Analyzer*)self);

    if (stoplist) {
        if (language) { THROW("Can't have both stoplist and language"); }
        self->stoplist = (Hash*)INCREF(stoplist);
    }
    else if (language) {
        self->stoplist = Stopalizer_gen_stoplist(language);
        if (!self->stoplist)
            THROW("Can't get a stoplist for '%o'", language);
    }
    else {
        THROW("Either stoplist or language is required");
    }

    return self;
}

void
Stopalizer_destroy(Stopalizer *self)
{
    DECREF(self->stoplist);
    FREE_OBJ(self);
}

Inversion*
Stopalizer_transform(Stopalizer *self, Inversion *inversion)
{
    Token *token;
    Inversion *new_inversion = Inversion_new(NULL);
    Hash *const stoplist  = self->stoplist;

    while (NULL != (token = Inversion_Next(inversion))) {
        if (!Hash_Fetch_Str(stoplist, token->text, token->len)) {
            Inversion_Append(new_inversion, (Token*)INCREF(token));
        }
    }

    return new_inversion;
}

bool_t
Stopalizer_dump_equals(Stopalizer *self, Obj *dump)
{
    Stopalizer_dump_equals_t super_dump_equals = (Stopalizer_dump_equals_t)
        SUPER_METHOD(&STOPALIZER, Stopalizer, Dump_Equals);
    if (!super_dump_equals(self, dump)) {
        return false;
    }
    else {
        Hash *source   = (Hash*)ASSERT_IS_A(dump, HASH);
        Hash *stoplist = (Hash*)Hash_Fetch_Str(source, "stoplist", 8);
        if (!stoplist) return false;
        if (!Hash_Equals(self->stoplist, (Obj*)stoplist)) return false;
    }
    return true;
}

bool_t
Stopalizer_equals(Stopalizer *self, Obj *other)
{
    Stopalizer *const evil_twin = (Stopalizer*)other;
    if (evil_twin == self) return true;
    if (!OBJ_IS_A(evil_twin, STOPALIZER)) return false;
    if (!Hash_Equals(evil_twin->stoplist, (Obj*)self->stoplist)) {
        return false;
    }
    return true;
}

/* Copyright 2005-2009 Marvin Humphrey
 *
 * This program is free software; you can redistribute it and/or modify
 * under the same terms as Perl itself.
 */

