#include "Search/Kino03/KinoSearch/Util/ToolSet.h"

#include "Search/Kino03/KinoSearch/Test.h"
#include "Search/Kino03/KinoSearch/Test/Index/TestDocWriter.h"
#include "Search/Kino03/KinoSearch/Index/DocWriter.h"

void
TestDocWriter_run_tests()
{
    TestBatch *batch = Test_new_batch("TestDocWriter", 1, NULL);
    PLAN(batch);
    PASS(batch, "placeholder");
    batch->destroy(batch);
}

/* Copyright 2005-2009 Marvin Humphrey
 *
 * This program is free software; you can redistribute it and/or modify
 * under the same terms as Perl itself.
 */

