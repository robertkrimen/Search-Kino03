#include "Search/Kino03/KinoSearch/Util/ToolSet.h"
#include <math.h>

#include "Search/Kino03/KinoSearch/Test.h"
#include "Search/Kino03/KinoSearch/Test/TestUtils.h"
#include "Search/Kino03/KinoSearch/Test/Search/TestPolyQuery.h"
#include "Search/Kino03/KinoSearch/Search/ANDQuery.h"
#include "Search/Kino03/KinoSearch/Search/ORQuery.h"
#include "Search/Kino03/KinoSearch/Search/PolyQuery.h"
#include "Search/Kino03/KinoSearch/Search/LeafQuery.h"

static void
test_Dump_Load_and_Equals(TestBatch *batch, u32_t boolop)
{
    LeafQuery *a_leaf  = TestUtils_make_leaf_query(NULL, "a");
    LeafQuery *b_leaf  = TestUtils_make_leaf_query(NULL, "b");
    LeafQuery *c_leaf  = TestUtils_make_leaf_query(NULL, "c");
    PolyQuery *query   = (PolyQuery*)TestUtils_make_poly_query(boolop, 
        INCREF(a_leaf), INCREF(b_leaf), NULL);
    PolyQuery *kids_differ = (PolyQuery*)TestUtils_make_poly_query(boolop, 
        INCREF(a_leaf), INCREF(b_leaf), INCREF(c_leaf), NULL);
    PolyQuery *boost_differs = (PolyQuery*)TestUtils_make_poly_query(boolop, 
        INCREF(a_leaf), INCREF(b_leaf), NULL);
    Obj     *dump  = (Obj*)PolyQuery_Dump(query);
    PolyQuery *clone = (PolyQuery*)Obj_Load(dump, dump);

    ASSERT_FALSE(batch, PolyQuery_Equals(query, (Obj*)kids_differ), 
        "Different kids spoil Equals");
    ASSERT_TRUE(batch, PolyQuery_Equals(query, (Obj*)boost_differs), 
        "Equals with identical boosts");
    PolyQuery_Set_Boost(boost_differs, 1.5);
    ASSERT_FALSE(batch, PolyQuery_Equals(query, (Obj*)boost_differs), 
        "Different boost spoils Equals");
    ASSERT_TRUE(batch, PolyQuery_Equals(query, (Obj*)clone), 
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
TestANDQuery_run_tests()
{
    TestBatch *batch = Test_new_batch("TestANDQuery", 4, NULL);
    PLAN(batch);
    test_Dump_Load_and_Equals(batch, BOOLOP_AND);
    batch->destroy(batch);
}

void
TestORQuery_run_tests()
{
    TestBatch *batch = Test_new_batch("TestORQuery", 4, NULL);
    PLAN(batch);
    test_Dump_Load_and_Equals(batch, BOOLOP_OR);
    batch->destroy(batch);
}

/* Copyright 2005-2009 Marvin Humphrey
 *
 * This program is free software; you can redistribute it and/or modify
 * under the same terms as Perl itself.
 */

