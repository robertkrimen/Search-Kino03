#include "Search/Kino03/KinoSearch/Util/ToolSet.h"
#include <math.h>

#include "Search/Kino03/KinoSearch/Test.h"
#include "Search/Kino03/KinoSearch/Test/TestUtils.h"
#include "Search/Kino03/KinoSearch/Test/Search/TestReqOptQuery.h"
#include "Search/Kino03/KinoSearch/Search/RequiredOptionalQuery.h"
#include "Search/Kino03/KinoSearch/Search/LeafQuery.h"

static void
test_Dump_Load_and_Equals(TestBatch *batch)
{
    Query *a_leaf  = (Query*)TestUtils_make_leaf_query(NULL, "a");
    Query *b_leaf  = (Query*)TestUtils_make_leaf_query(NULL, "b");
    Query *c_leaf  = (Query*)TestUtils_make_leaf_query(NULL, "c");
    RequiredOptionalQuery *query = ReqOptQuery_new(a_leaf, b_leaf);
    RequiredOptionalQuery *kids_differ = ReqOptQuery_new(a_leaf, c_leaf);
    RequiredOptionalQuery *boost_differs = ReqOptQuery_new(a_leaf, b_leaf);
    Obj *dump = (Obj*)ReqOptQuery_Dump(query);
    RequiredOptionalQuery *clone = (RequiredOptionalQuery*)Obj_Load(dump, dump);

    ASSERT_FALSE(batch, ReqOptQuery_Equals(query, (Obj*)kids_differ), 
        "Different kids spoil Equals");
    ASSERT_TRUE(batch, ReqOptQuery_Equals(query, (Obj*)boost_differs), 
        "Equals with identical boosts");
    ReqOptQuery_Set_Boost(boost_differs, 1.5);
    ASSERT_FALSE(batch, ReqOptQuery_Equals(query, (Obj*)boost_differs), 
        "Different boost spoils Equals");
    ASSERT_TRUE(batch, ReqOptQuery_Equals(query, (Obj*)clone), 
        "Dump => Load round trip");

    DECREF(a_leaf);
    DECREF(b_leaf);
    DECREF(c_leaf);
    DECREF(query);
    DECREF(kids_differ);
    DECREF(boost_differs);
    DECREF(dump);
    DECREF(clone);
}

void
TestReqOptQuery_run_tests()
{
    TestBatch *batch = Test_new_batch("TestReqOptQuery", 4, NULL);
    PLAN(batch);
    test_Dump_Load_and_Equals(batch);
    batch->destroy(batch);
}

/* Copyright 2005-2009 Marvin Humphrey
 *
 * This program is free software; you can redistribute it and/or modify
 * under the same terms as Perl itself.
 */

