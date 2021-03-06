#include "Search/Kino03/KinoSearch/Util/ToolSet.h"
#include <math.h>

#include "Search/Kino03/KinoSearch/Test.h"
#include "Search/Kino03/KinoSearch/Test/TestUtils.h"
#include "Search/Kino03/KinoSearch/Test/Search/TestPhraseQuery.h"
#include "Search/Kino03/KinoSearch/Search/PhraseQuery.h"

static void
test_Dump_And_Load(TestBatch *batch)
{
    PhraseQuery *query 
        = TestUtils_make_phrase_query("content", "a", "b", "c", NULL);
    Obj         *dump  = (Obj*)PhraseQuery_Dump(query);
    PhraseQuery *evil_twin = (PhraseQuery*)Obj_Load(dump, dump);
    ASSERT_TRUE(batch, PhraseQuery_Equals(query, (Obj*)evil_twin), 
        "Dump => Load round trip");
    DECREF(query);
    DECREF(dump);
    DECREF(evil_twin);
}

void
TestPhraseQuery_run_tests()
{
    TestBatch *batch = Test_new_batch("TestPhraseQuery", 1, NULL);
    PLAN(batch);
    test_Dump_And_Load(batch);
    batch->destroy(batch);
}

/* Copyright 2005-2009 Marvin Humphrey
 *
 * This program is free software; you can redistribute it and/or modify
 * under the same terms as Perl itself.
 */

