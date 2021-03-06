#include "Search/Kino03/KinoSearch/Util/ToolSet.h"
#include <math.h>

#include "Search/Kino03/KinoSearch/Test.h"
#include "Search/Kino03/KinoSearch/Test/TestUtils.h"
#include "Search/Kino03/KinoSearch/Test/Search/TestRangeQuery.h"
#include "Search/Kino03/KinoSearch/Search/RangeQuery.h"

static void
test_Dump_Load_and_Equals(TestBatch *batch)
{
    RangeQuery *query = TestUtils_make_range_query("content", "foo", "phooey",
        true, true);
    RangeQuery *lower_term_differs = TestUtils_make_range_query("content", 
        "goo", "phooey", true, true);
    RangeQuery *upper_term_differs = TestUtils_make_range_query("content", 
        "foo", "gooey", true, true);
    RangeQuery *include_lower_differs = TestUtils_make_range_query("content", 
        "foo", "phooey", false, true);
    RangeQuery *include_upper_differs = TestUtils_make_range_query("content", 
        "foo", "phooey", true, false);
    Obj        *dump    = (Obj*)RangeQuery_Dump(query);
    RangeQuery *clone   = (RangeQuery*)RangeQuery_Load(lower_term_differs, dump);

    ASSERT_FALSE(batch, RangeQuery_Equals(query, (Obj*)lower_term_differs),
        "Equals() false with different lower term");
    ASSERT_FALSE(batch, RangeQuery_Equals(query, (Obj*)upper_term_differs),
        "Equals() false with different upper term");
    ASSERT_FALSE(batch, RangeQuery_Equals(query, (Obj*)include_lower_differs),
        "Equals() false with different include_lower");
    ASSERT_FALSE(batch, RangeQuery_Equals(query, (Obj*)include_upper_differs),
        "Equals() false with different include_upper");
    ASSERT_TRUE(batch, RangeQuery_Equals(query, (Obj*)clone), 
        "Dump => Load round trip");

    DECREF(query);
    DECREF(lower_term_differs);
    DECREF(upper_term_differs);
    DECREF(include_lower_differs);
    DECREF(include_upper_differs);
    DECREF(dump);
    DECREF(clone);
}


void
TestRangeQuery_run_tests()
{
    TestBatch *batch = Test_new_batch("TestRangeQuery", 5, NULL);
    PLAN(batch);
    test_Dump_Load_and_Equals(batch);
    batch->destroy(batch);
}

/* Copyright 2005-2009 Marvin Humphrey
 *
 * This program is free software; you can redistribute it and/or modify
 * under the same terms as Perl itself.
 */

