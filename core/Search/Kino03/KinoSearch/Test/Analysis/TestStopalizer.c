#include "Search/Kino03/KinoSearch/Util/ToolSet.h"
#include <stdarg.h>

#include "Search/Kino03/KinoSearch/Test.h"
#include "Search/Kino03/KinoSearch/Test/Analysis/TestStopalizer.h"
#include "Search/Kino03/KinoSearch/Analysis/Stopalizer.h"

static Stopalizer* 
S_make_stopalizer(void *unused, ...)
{
    va_list args;
    Stopalizer *self = (Stopalizer*)VTable_Make_Obj(&STOPALIZER);
    Hash *stoplist = Hash_new(0);
    char *stopword;

    va_start(args, unused);
    while (NULL != (stopword = va_arg(args, char*))) {
        Hash_Store_Str(stoplist, stopword, strlen(stopword), INCREF(&EMPTY));
    }
    va_end(args);

    self = Stopalizer_init(self, NULL, stoplist);
    DECREF(stoplist);
    return self;
}

static void
test_Dump_Load_and_Equals(TestBatch *batch)
{
    Stopalizer *stopalizer 
        = S_make_stopalizer(NULL, "foo", "bar", "baz", NULL);
    Stopalizer *other       = S_make_stopalizer(NULL, "foo", "bar", NULL);
    Obj        *dump        = Stopalizer_Dump(stopalizer);
    Obj        *other_dump  = Stopalizer_Dump(other);
    Stopalizer *clone       = (Stopalizer*)Stopalizer_Load(other, dump);
    Stopalizer *other_clone = (Stopalizer*)Stopalizer_Load(other, other_dump);

    ASSERT_FALSE(batch, Stopalizer_Equals(stopalizer,
        (Obj*)other), "Equals() false with different stoplist");
    ASSERT_TRUE(batch, Stopalizer_Dump_Equals(other,
        (Obj*)other_dump), "Dump_Equals()");
    ASSERT_TRUE(batch, Stopalizer_Dump_Equals(stopalizer,
        (Obj*)dump), "Dump_Equals()");
    ASSERT_FALSE(batch, Stopalizer_Dump_Equals(stopalizer,
        (Obj*)other_dump), "Dump_Equals() false with different stoplist");
    ASSERT_FALSE(batch, Stopalizer_Dump_Equals(other,
        (Obj*)dump), "Dump_Equals() false with different stoplist");
    ASSERT_TRUE(batch, Stopalizer_Equals(stopalizer,
        (Obj*)clone), "Dump => Load round trip");
    ASSERT_TRUE(batch, Stopalizer_Equals(other,
        (Obj*)other_clone), "Dump => Load round trip");

    DECREF(stopalizer);
    DECREF(dump);
    DECREF(clone);
    DECREF(other);
    DECREF(other_dump);
    DECREF(other_clone);
}



void
TestStopalizer_run_tests()
{
    TestBatch *batch = Test_new_batch("TestStopalizer", 7, NULL);

    PLAN(batch);

    test_Dump_Load_and_Equals(batch);

    batch->destroy(batch);
}


/* Copyright 2005-2009 Marvin Humphrey
 *
 * This program is free software; you can redistribute it and/or modify
 * under the same terms as Perl itself.
 */

