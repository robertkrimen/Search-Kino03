#include "Search/Kino03/KinoSearch/Util/ToolSet.h"
#include <stdarg.h>
#include <string.h>

#include "Search/Kino03/KinoSearch/Test/TestQueryParser.h"
#include "Search/Kino03/KinoSearch/Test/TestUtils.h"
#include "Search/Kino03/KinoSearch/Search/TermQuery.h"
#include "Search/Kino03/KinoSearch/Search/PhraseQuery.h"
#include "Search/Kino03/KinoSearch/Search/LeafQuery.h"
#include "Search/Kino03/KinoSearch/Search/ANDQuery.h"
#include "Search/Kino03/KinoSearch/Search/NOTQuery.h"
#include "Search/Kino03/KinoSearch/Search/ORQuery.h"

TestQueryParser*
TestQP_new(const char *query_string, Query *tree, Query *expanded, 
           u32_t num_hits)
{
    TestQueryParser *self = (TestQueryParser*)VTable_Make_Obj(&TESTQUERYPARSER);
    return TestQP_init(self, query_string, tree, expanded, num_hits);
}

TestQueryParser*
TestQP_init(TestQueryParser *self, const char *query_string, Query *tree, 
            Query *expanded, u32_t num_hits)
{
    self->query_string = query_string ? TestUtils_get_cb(query_string) : NULL;
    self->tree         = tree     ? tree     : NULL;
    self->expanded     = expanded ? expanded : NULL;
    self->num_hits     = num_hits;
    return self;
}

void
TestQP_destroy(TestQueryParser *self)
{
    DECREF(self->query_string);
    DECREF(self->tree);
    DECREF(self->expanded);
    FREE_OBJ(self);
}

CharBuf*
TestQP_get_query_string(TestQueryParser *self) { return self->query_string; }
Query*
TestQP_get_tree(TestQueryParser *self)         { return self->tree; }
Query*
TestQP_get_expanded(TestQueryParser *self)     { return self->expanded; }
u32_t
TestQP_get_num_hits(TestQueryParser *self)     { return self->num_hits; }


/* Copyright 2005-2009 Marvin Humphrey
 *
 * This program is free software; you can redistribute it and/or modify
 * under the same terms as Perl itself.
 */

