#include "Search/Kino03/KinoSearch/Util/ToolSet.h"

#include "Search/Kino03/KinoSearch/Search/Searchable.h"

#include "Search/Kino03/KinoSearch/Index/DocVector.h"
#include "Search/Kino03/KinoSearch/QueryParser.h"
#include "Search/Kino03/KinoSearch/Search/HitCollector.h"
#include "Search/Kino03/KinoSearch/Search/Hits.h"
#include "Search/Kino03/KinoSearch/Search/NoMatchQuery.h"
#include "Search/Kino03/KinoSearch/Search/Query.h"
#include "Search/Kino03/KinoSearch/Search/SortSpec.h"
#include "Search/Kino03/KinoSearch/Search/TopDocs.h"
#include "Search/Kino03/KinoSearch/Search/Compiler.h"
#include "Search/Kino03/KinoSearch/Schema.h"

Searchable*
Searchable_init(Searchable *self, Schema *schema)
{
    self->schema  = (Schema*)INCREF(schema);
    self->qparser = NULL;
    ABSTRACT_CLASS_CHECK(self, SEARCHABLE);
    return self;
}

void
Searchable_destroy(Searchable *self)
{
    DECREF(self->schema);
    DECREF(self->qparser);
    FREE_OBJ(self);
}

Hits*
Searchable_hits(Searchable *self, Obj *query, u32_t offset, 
                u32_t num_wanted, SortSpec *sort_spec)
{
    Query   *real_query = Searchable_Glean_Query(self, query);
    TopDocs *top_docs   = Searchable_Top_Docs(self, real_query, 
                                offset + num_wanted, sort_spec);
    Hits    *hits       = Hits_new(self, top_docs, offset);
    DECREF(top_docs);
    DECREF(real_query);
    return hits;
}

Query*
Searchable_glean_query(Searchable *self, Obj *query)
{
    Query *real_query = NULL;

    if (!query) {
        real_query = (Query*)NoMatchQuery_new();
    }
    else if (OBJ_IS_A(query, QUERY)) {
        real_query = (Query*)INCREF(query);
    }
    else if (OBJ_IS_A(query, CHARBUF)) {
        if (!self->qparser) 
            self->qparser = QParser_new(self->schema, NULL, NULL, NULL);
        real_query = QParser_Parse(self->qparser, (CharBuf*)query);
    }
    else {
        THROW("Invalid type for 'query' param: %o",
            Obj_Get_Class_Name(query));
    }

    return real_query;
}

Schema*
Searchable_get_schema(Searchable *self)
{
    return self->schema;
}

void
Searchable_close(Searchable *self)
{
    UNUSED_VAR(self);
}

/* Copyright 2006-2009 Marvin Humphrey
 *
 * This program is free software; you can redistribute it and/or modify
 * under the same terms as Perl itself.
 */

