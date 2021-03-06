#include "Search/Kino03/KinoSearch/Util/ToolSet.h"

#include "Search/Kino03/KinoSearch/Test.h"
#include "Search/Kino03/KinoSearch/Test/Util/TestPriorityQueue.h"
#include "Search/Kino03/KinoSearch/Util/PriorityQueue.h"

NumPriorityQueue*
NumPriQ_new(u32_t max_size)
{
    NumPriorityQueue *self 
        = (NumPriorityQueue*)VTable_Make_Obj(&NUMPRIORITYQUEUE);
    return (NumPriorityQueue*)PriQ_init((PriorityQueue*)self, max_size);
}

bool_t
NumPriQ_less_than(NumPriorityQueue *self, Obj *a, Obj *b)
{
    Float64 *num_a = (Float64*)a;
    Float64 *num_b = (Float64*)b;
    UNUSED_VAR(self);
    return Float64_Get_Value(num_a) < Float64_Get_Value(num_b) ? true : false;
}

static void
S_insert_num(NumPriorityQueue *pq, i32_t value)
{
    NumPriQ_Insert(pq, (Obj*)Float64_new((double)value));
}

static i32_t
S_pop_num(NumPriorityQueue *pq)
{
    Float64 *num = (Float64*)PriQ_Pop(pq);
    i32_t retval;
    if (!num) { THROW("Queue is empty"); }
    retval = (i32_t)Float64_Get_Value(num);
    DECREF(num);
    return retval;
}

static void
test_Peek_and_Pop_All(TestBatch *batch)
{
    NumPriorityQueue *pq = NumPriQ_new(5);

    S_insert_num(pq, 3);
    S_insert_num(pq, 1);
    S_insert_num(pq, 2);
    S_insert_num(pq, 20);
    S_insert_num(pq, 10);
    ASSERT_INT_EQ(batch, (long)Float64_Get_Value(PriQ_Peek(pq)), 1, 
        "peek at the least item in the queue" );
    {
        VArray *got = PriQ_Pop_All(pq);
        ASSERT_INT_EQ(batch, (long)Float64_Get_Value(VA_Fetch(got, 0)), 20, 
            "pop_all");
        ASSERT_INT_EQ(batch, (long)Float64_Get_Value(VA_Fetch(got, 1)), 10, 
            "pop_all");
        ASSERT_INT_EQ(batch, (long)Float64_Get_Value(VA_Fetch(got, 2)), 3, 
            "pop_all");
        ASSERT_INT_EQ(batch, (long)Float64_Get_Value(VA_Fetch(got, 3)), 2, 
            "pop_all");
        ASSERT_INT_EQ(batch, (long)Float64_Get_Value(VA_Fetch(got, 4)), 1, 
            "pop_all");
        DECREF(got);
    }

    DECREF(pq);
}

static void
test_Insert_and_Pop(TestBatch *batch)
{
    NumPriorityQueue *pq = NumPriQ_new(5);

    S_insert_num(pq, 3);
    S_insert_num(pq, 1);
    S_insert_num(pq, 2);
    S_insert_num(pq, 20);
    S_insert_num(pq, 10);

    ASSERT_INT_EQ(batch, S_pop_num(pq), 1, "Pop"); 
    ASSERT_INT_EQ(batch, S_pop_num(pq), 2, "Pop"); 
    ASSERT_INT_EQ(batch, S_pop_num(pq), 3, "Pop"); 
    ASSERT_INT_EQ(batch, S_pop_num(pq), 10, "Pop"); 

    S_insert_num(pq, 7);
    ASSERT_INT_EQ(batch, S_pop_num(pq), 7, 
        "Insert after Pop still sorts correctly"); 

    DECREF(pq);
}

static void
test_discard(TestBatch *batch)
{
    i32_t i;
    NumPriorityQueue *pq = NumPriQ_new(5);

    for (i = 1; i <= 10; i++) { S_insert_num(pq, i); }
    S_insert_num(pq, -3);
    for (i = 1590; i <= 1600; i++) { S_insert_num(pq, i); }
    S_insert_num(pq, 5);

    ASSERT_INT_EQ(batch, S_pop_num(pq), 1596, "discard waste"); 
    ASSERT_INT_EQ(batch, S_pop_num(pq), 1597, "discard waste"); 
    ASSERT_INT_EQ(batch, S_pop_num(pq), 1598, "discard waste"); 
    ASSERT_INT_EQ(batch, S_pop_num(pq), 1599, "discard waste"); 
    ASSERT_INT_EQ(batch, S_pop_num(pq), 1600, "discard waste"); 

    DECREF(pq);
}

static void
test_random_insertion(TestBatch *batch)
{
    int i;
    int shuffled[64];
    NumPriorityQueue *pq = NumPriQ_new(64);

    for (i = 0; i < 64; i++) { shuffled[i] = i; }
    for (i = 0; i < 64; i++) { 
        int shuffle_pos = rand() % 64;
        int temp = shuffled[shuffle_pos];
        shuffled[shuffle_pos] = shuffled[i];
        shuffled[i] = temp; 
    }
    for (i = 0; i < 64; i++) { S_insert_num(pq, shuffled[i]); }
    for (i = 0; i < 64; i++) { 
        if (S_pop_num(pq) != i) { break; }
    }
    ASSERT_INT_EQ(batch, i, 64, "random insertion");

    DECREF(pq);
}

void
TestPriQ_run_tests()
{
    TestBatch   *batch     = Test_new_batch("TestPriQ", 17, NULL);

    PLAN(batch);

    test_Peek_and_Pop_All(batch);
    test_Insert_and_Pop(batch);
    test_discard(batch);
    test_random_insertion(batch);

    batch->destroy(batch);
}


/* Copyright 2005-2009 Marvin Humphrey
 *
 * This program is free software; you can redistribute it and/or modify
 * under the same terms as Perl itself.
 */

