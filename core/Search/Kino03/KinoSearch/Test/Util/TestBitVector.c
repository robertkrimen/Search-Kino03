#include "Search/Kino03/KinoSearch/Util/ToolSet.h"

#include "Search/Kino03/KinoSearch/Test.h"
#include "Search/Kino03/KinoSearch/Test/Util/TestBitVector.h"
#include "Search/Kino03/KinoSearch/Util/BitVector.h"

static void
test_Set_and_Get(TestBatch *batch)
{
    unsigned i, max;
    const u32_t three      = 3;
    const u32_t seventeen  = 17;
    BitVector *bit_vec     = BitVec_new(8);

    BitVec_Set(bit_vec, three);
    ASSERT_TRUE(batch, BitVec_Get_Cap(bit_vec) < seventeen, "set below cap");
    BitVec_Set(bit_vec, seventeen);
    ASSERT_TRUE(batch, BitVec_Get_Cap(bit_vec) > seventeen, 
        "set above cap causes BitVector to grow");

    for (i = 0, max = BitVec_Get_Cap(bit_vec); i < max; i++) {
        if (i == three || i == seventeen) { 
            ASSERT_TRUE(batch, BitVec_Get(bit_vec, i), "set/get %d", i); 
        }
        else {
            ASSERT_FALSE(batch, BitVec_Get(bit_vec, i), "get %d", i); 
        }
    }
    ASSERT_FALSE(batch, BitVec_Get(bit_vec, i), "out of range get");

    DECREF(bit_vec);
}

static void
test_Flip(TestBatch *batch)
{
    BitVector *bit_vec = BitVec_new(0);
    int i;

    for (i = 0; i <= 20; i++) { BitVec_Flip(bit_vec, i); }
    for (i = 0; i <= 20; i++) {
        ASSERT_TRUE(batch, BitVec_Get(bit_vec, i), "flip on %d", i);
    }
    ASSERT_FALSE(batch, BitVec_Get(bit_vec, i), "no flip %d", i);
    for (i = 0; i <= 20; i++) { BitVec_Flip(bit_vec, i); }
    for (i = 0; i <= 20; i++) {
        ASSERT_FALSE(batch, BitVec_Get(bit_vec, i), "flip off %d", i);
    }
    ASSERT_FALSE(batch, BitVec_Get(bit_vec, i), "still no flip %d", i);

    DECREF(bit_vec);
}

static void
test_Flip_Block_ascending(TestBatch *batch)
{
    BitVector *bit_vec = BitVec_new(0);
    int i;

    for (i = 0; i <= 20; i++) {
        BitVec_Flip_Block(bit_vec, i, 21 - i);
    }

    for (i = 0; i <= 20; i++) {
        if (i % 2 == 0) { 
            ASSERT_TRUE(batch, BitVec_Get(bit_vec, i), 
                "Flip_Block ascending %d", i);
        }
        else {
            ASSERT_FALSE(batch, BitVec_Get(bit_vec, i), 
                "Flip_Block ascending %d", i);
        }
    }

    DECREF(bit_vec);
}

static void
test_Flip_Block_descending(TestBatch *batch)
{
    BitVector *bit_vec = BitVec_new(0);
    int i;

    for (i = 19; i >= 0; i--) {
        BitVec_Flip_Block(bit_vec, 1, i);
    }

    for (i = 0; i <= 20; i++) {
        if (i % 2) { 
            ASSERT_TRUE(batch, BitVec_Get(bit_vec, i), 
                "Flip_Block descending %d", i);
        }
        else {
            ASSERT_FALSE(batch, BitVec_Get(bit_vec, i), 
                "Flip_Block descending %d", i);
        }
    }

    DECREF(bit_vec);
}

static void
test_Flip_Block_bulk(TestBatch *batch)
{
    i32_t offset;

    for (offset = 0; offset <= 17; offset++) {
        i32_t len;
        for (len = 0; len <= 17; len++) {
            int i;
            int upper = offset + len - 1;
            BitVector *bit_vec = BitVec_new(0);

            BitVec_Flip_Block(bit_vec, offset, len);
            for (i = 0; i <= 17; i++) {
                if (i >= offset && i <= upper) {
                    if (!BitVec_Get(bit_vec, i)) { break; }
                }
                else {
                    if (BitVec_Get(bit_vec, i)) { break; }
                }
            }
            ASSERT_INT_EQ(batch, i, 18, "Flip_Block(%d, %d)", offset, len);

            DECREF(bit_vec);
        }
    }
}

static void
test_Copy(TestBatch *batch)
{
    int foo;

    for (foo = 0; foo <= 17; foo++) {
        int bar;
        for (bar = 0; bar <= 17; bar++) {
            int i;
            BitVector *foo_vec = BitVec_new(0);
            BitVector *bar_vec = BitVec_new(0);

            BitVec_Set(foo_vec, foo);
            BitVec_Set(bar_vec, bar);
            BitVec_Copy(foo_vec, bar_vec);

            for (i = 0; i <= 17; i++) {
                if (BitVec_Get(foo_vec, i) && i != bar) { break; }
            }
            ASSERT_INT_EQ(batch, i, 18, "Copy(%d, %d)", foo, bar);

            DECREF(foo_vec);
            DECREF(bar_vec);
        }
    }
}

static BitVector*
S_create_set(int set_num) 
{
    int i;
    int nums_1[] = { 1, 2, 3, 10, 20, 30, 0 };
    int nums_2[] = { 2, 3, 4, 5, 6, 7, 8, 9, 10, 
                     25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 0 };
    int *nums = set_num == 1 ? nums_1 : nums_2;
    BitVector *bit_vec = BitVec_new(31);
    for (i = 0; nums[i] != 0; i++) {
        BitVec_Set(bit_vec, nums[i]);
    }
    return bit_vec;
}

#define OP_OR 1
#define OP_XOR 2
#define OP_AND 3
#define OP_AND_NOT 4
static int
S_verify_logical_op(BitVector *bit_vec, BitVector *set_1, BitVector *set_2, 
                  int op)
{
    int i;

    for (i = 0; i < 50; i++) {
        bool_t wanted;
        
        switch(op) {
            case OP_OR:  
                wanted = BitVec_Get(set_1, i) || BitVec_Get(set_2, i);
                break;
            case OP_XOR:
                wanted = (!BitVec_Get(set_1, i)) != (!BitVec_Get(set_2, i));
                break;
            case OP_AND:
                wanted = BitVec_Get(set_1, i) && BitVec_Get(set_2, i);
                break;
            case OP_AND_NOT:
                wanted = BitVec_Get(set_1, i) && (!BitVec_Get(set_2, i));
                break;
            default:
                wanted = false;
                THROW("unknown op: %d", op);
        }
                      
        if (BitVec_Get(bit_vec, i) != wanted) { break; }
    }

    return i;
}

static void
test_Or(TestBatch *batch)
{
    BitVector *smaller = S_create_set(1);
    BitVector *larger  = S_create_set(2);
    BitVector *set_1   = S_create_set(1);
    BitVector *set_2   = S_create_set(2);

    BitVec_Or(smaller, set_2);
    ASSERT_INT_EQ(batch, S_verify_logical_op(smaller, set_1, set_2, OP_OR), 50, 
        "OR with self smaller than other");
    BitVec_Or(larger, set_1);
    ASSERT_INT_EQ(batch, S_verify_logical_op(larger, set_1, set_2, OP_OR), 50, 
        "OR with other smaller than self");

    DECREF(smaller);
    DECREF(larger);
    DECREF(set_1);
    DECREF(set_2);
}

static void
test_Xor(TestBatch *batch)
{
    BitVector *smaller = S_create_set(1);
    BitVector *larger  = S_create_set(2);
    BitVector *set_1   = S_create_set(1);
    BitVector *set_2   = S_create_set(2);

    BitVec_Xor(smaller, set_2);
    ASSERT_INT_EQ(batch, S_verify_logical_op(smaller, set_1, set_2, OP_XOR), 
        50, "XOR with self smaller than other");
    BitVec_Xor(larger, set_1);
    ASSERT_INT_EQ(batch, S_verify_logical_op(larger, set_1, set_2, OP_XOR), 
        50, "XOR with other smaller than self");

    DECREF(smaller);
    DECREF(larger);
    DECREF(set_1);
    DECREF(set_2);
}

static void
test_And(TestBatch *batch)
{
    BitVector *smaller = S_create_set(1);
    BitVector *larger  = S_create_set(2);
    BitVector *set_1   = S_create_set(1);
    BitVector *set_2   = S_create_set(2);

    BitVec_And(smaller, set_2);
    ASSERT_INT_EQ(batch, S_verify_logical_op(smaller, set_1, set_2, OP_AND), 
        50, "AND with self smaller than other");
    BitVec_And(larger, set_1);
    ASSERT_INT_EQ(batch, S_verify_logical_op(larger, set_1, set_2, OP_AND), 
        50, "AND with other smaller than self");

    DECREF(smaller);
    DECREF(larger);
    DECREF(set_1);
    DECREF(set_2);
}

static void
test_And_Not(TestBatch *batch)
{
    BitVector *smaller = S_create_set(1);
    BitVector *larger  = S_create_set(2);
    BitVector *set_1   = S_create_set(1);
    BitVector *set_2   = S_create_set(2);

    BitVec_And_Not(smaller, set_2);
    ASSERT_INT_EQ(batch, S_verify_logical_op(smaller, set_1, set_2, OP_AND_NOT), 
        50, "AND_NOT with self smaller than other");
    BitVec_And_Not(larger, set_1);
    ASSERT_INT_EQ(batch, S_verify_logical_op(larger, set_2, set_1, OP_AND_NOT),
        50, "AND_NOT with other smaller than self");

    DECREF(smaller);
    DECREF(larger);
    DECREF(set_1);
    DECREF(set_2);
}

static void
test_Count(TestBatch *batch)
{
    int i;
    int shuffled[64];
    BitVector *bit_vec = BitVec_new(64);

    for (i = 0; i < 64; i++) { shuffled[i] = i; }
    for (i = 0; i < 64; i++) { 
        int shuffle_pos = rand() % 64;
        int temp = shuffled[shuffle_pos];
        shuffled[shuffle_pos] = shuffled[i];
        shuffled[i] = temp; 
    }
    for (i = 0; i < 64; i++) { 
        BitVec_Set(bit_vec, shuffled[i]);
        if (BitVec_Count(bit_vec) != (u32_t)(i + 1)) { break; }
    }
    ASSERT_INT_EQ(batch, i, 64, "Count() returns the right number of bits");

    DECREF(bit_vec);
}

static void
test_Next_Set_Bit(TestBatch *batch)
{
    int i;

    for (i = 24; i <= 33; i++) {
        int probe;
        BitVector *bit_vec = BitVec_new(64);
        BitVec_Set(bit_vec, i);
        ASSERT_INT_EQ(batch, BitVec_Next_Set_Bit(bit_vec, 0), i, 
            "Next_Set_Bit for 0 is %d", i);
        ASSERT_INT_EQ(batch, BitVec_Next_Set_Bit(bit_vec, 0), i, 
            "Next_Set_Bit for 1 is %d", i);
        for (probe = 15; probe <= i; probe++) {
            ASSERT_INT_EQ(batch, BitVec_Next_Set_Bit(bit_vec, probe), i, 
                "Next_Set_Bit for %d is %d", probe, i);
        }
        for (probe = i + 1; probe <= i + 9; probe++) {
            ASSERT_INT_EQ(batch, BitVec_Next_Set_Bit(bit_vec, probe), -1,
                "no Next_Set_Bit for %d when max is %d", probe, i );
        }
        DECREF(bit_vec);
    }
}

static void
test_Clear_All(TestBatch *batch)
{
    BitVector *bit_vec = BitVec_new(64);
    BitVec_Flip_Block(bit_vec, 0, 63);
    BitVec_Clear_All(bit_vec);
    ASSERT_INT_EQ(batch, BitVec_Next_Set_Bit(bit_vec, 0), -1, "Clear_All");
    DECREF(bit_vec);
}

static void
test_Clone(TestBatch *batch)
{
    int i;
    BitVector *self = BitVec_new(30);
    BitVector *evil_twin;

    BitVec_Set(self, 2);
    BitVec_Set(self, 3);
    BitVec_Set(self, 10);
    BitVec_Set(self, 20);

    evil_twin = BitVec_Clone(self);
    for (i = 0; i < 50; i++) {
        if (BitVec_Get(self, i) != BitVec_Get(evil_twin, i)) { break; }
    }
    ASSERT_INT_EQ(batch, i, 50, "Clone");
    ASSERT_INT_EQ(batch, BitVec_Count(evil_twin), 4, "clone Count");

    DECREF(self);
    DECREF(evil_twin);
}

/* Valgrind only - detect off-by-one error. */
static void
test_off_by_one_error()
{
    int cap;
    for (cap = 5; cap <= 24; cap++) {
        BitVector *bit_vec = BitVec_new(cap);
        BitVec_Set(bit_vec, cap - 2);
        DECREF(bit_vec);
    }
}

void
TestBitVector_run_tests()
{
    TestBatch   *batch     = Test_new_batch("TestInStream", 1028, NULL);

    PLAN(batch);

    test_Set_and_Get(batch);
    test_Flip(batch);
    test_Flip_Block_ascending(batch);
    test_Flip_Block_descending(batch);
    test_Flip_Block_bulk(batch);
    test_Copy(batch);
    test_Or(batch);
    test_Xor(batch);
    test_And(batch);
    test_And_Not(batch);
    test_Count(batch);
    test_Next_Set_Bit(batch);
    test_Clear_All(batch);
    test_Clone(batch);
    test_off_by_one_error();

    batch->destroy(batch);
}


/* Copyright 2005-2009 Marvin Humphrey
 *
 * This program is free software; you can redistribute it and/or modify
 * under the same terms as Perl itself.
 */

