#include "Search/Kino03/KinoSearch/Util/ToolSet.h"
#include <stdarg.h>
#include <string.h>

#include "Search/Kino03/KinoSearch/Test.h"
#include "Search/Kino03/KinoSearch/Test/TestQueryParserLogic.h"
#include "Search/Kino03/KinoSearch/Test/TestQueryParser.h"
#include "Search/Kino03/KinoSearch/Test/TestSchema.h"
#include "Search/Kino03/KinoSearch/Test/TestUtils.h"
#include "Search/Kino03/KinoSearch/Analysis/Analyzer.h"
#include "Search/Kino03/KinoSearch/Analysis/Tokenizer.h"
#include "Search/Kino03/KinoSearch/Doc.h"
#include "Search/Kino03/KinoSearch/Indexer.h"
#include "Search/Kino03/KinoSearch/QueryParser.h"
#include "Search/Kino03/KinoSearch/Searcher.h"
#include "Search/Kino03/KinoSearch/Search/Hits.h"
#include "Search/Kino03/KinoSearch/Search/TermQuery.h"
#include "Search/Kino03/KinoSearch/Search/PhraseQuery.h"
#include "Search/Kino03/KinoSearch/Search/LeafQuery.h"
#include "Search/Kino03/KinoSearch/Search/ANDQuery.h"
#include "Search/Kino03/KinoSearch/Search/MatchAllQuery.h"
#include "Search/Kino03/KinoSearch/Search/NOTQuery.h"
#include "Search/Kino03/KinoSearch/Search/NoMatchQuery.h"
#include "Search/Kino03/KinoSearch/Search/ORQuery.h"
#include "Search/Kino03/KinoSearch/Search/RequiredOptionalQuery.h"
#include "Search/Kino03/KinoSearch/Store/RAMFolder.h"

#define make_leaf_query   (Query*)kino_TestUtils_make_leaf_query
#define make_not_query    (Query*)kino_TestUtils_make_not_query
#define make_poly_query   (Query*)kino_TestUtils_make_poly_query

static TestQueryParser*
logical_test_empty_phrase(u32_t boolop)
{
    Query   *tree = make_leaf_query(NULL, "\"\"");
    UNUSED_VAR(boolop);
    return TestQP_new("\"\"", tree, NULL, 0);
}

static TestQueryParser*
logical_test_empty_parens(u32_t boolop)
{
    Query   *tree   = make_poly_query(boolop, NULL);
    return TestQP_new("()", tree, NULL, 0);
}

static TestQueryParser*
logical_test_nested_empty_parens(u32_t boolop)
{
    Query   *inner   = make_poly_query(boolop, NULL);
    Query   *tree    = make_poly_query(boolop, inner, NULL);
    return TestQP_new("(())", tree, NULL, 0);
}

static TestQueryParser*
logical_test_nested_empty_phrase(u32_t boolop)
{
    Query   *leaf   = make_leaf_query(NULL, "\"\"");
    Query   *tree   = make_poly_query(boolop, leaf, NULL);
    return TestQP_new("(\"\")", tree, NULL, 0);
}

static TestQueryParser*
logical_test_simple_term(u32_t boolop)
{
    Query   *tree   = make_leaf_query(NULL, "b");
    UNUSED_VAR(boolop);
    return TestQP_new("b", tree, NULL, 3);
}

static TestQueryParser*
logical_test_one_nested_term(u32_t boolop)
{
    Query   *leaf   = make_leaf_query(NULL, "a");
    Query   *tree   = make_poly_query(boolop, leaf, NULL);
    return TestQP_new("(a)", tree, NULL, 4);
}

static TestQueryParser*
logical_test_one_term_phrase(u32_t boolop)
{
    Query   *tree   = make_leaf_query(NULL, "\"a\"");
    UNUSED_VAR(boolop);
    return TestQP_new("\"a\"", tree, NULL, 4);
}

static TestQueryParser*
logical_test_two_terms(u32_t boolop)
{
    Query   *a_leaf    = make_leaf_query(NULL, "a");
    Query   *b_leaf    = make_leaf_query(NULL, "b");
    Query   *tree      = make_poly_query(boolop, a_leaf, b_leaf, NULL); 
    u32_t    num_hits  = boolop == BOOLOP_OR ? 4 : 3;
    return TestQP_new("a b", tree, NULL, num_hits);
}

static TestQueryParser*
logical_test_two_terms_nested(u32_t boolop)
{
    Query   *a_leaf     = make_leaf_query(NULL, "a");
    Query   *b_leaf     = make_leaf_query(NULL, "b");
    Query   *tree       = make_poly_query(boolop, a_leaf, b_leaf, NULL); 
    u32_t    num_hits   = boolop == BOOLOP_OR ? 4 : 3;
    return TestQP_new("(a b)", tree, NULL, num_hits);
}

static TestQueryParser*
logical_test_one_term_one_single_term_phrase(u32_t boolop)
{
    Query   *a_leaf    = make_leaf_query(NULL, "a");
    Query   *b_leaf    = make_leaf_query(NULL, "\"b\"");
    Query   *tree      = make_poly_query(boolop, a_leaf, b_leaf, NULL); 
    u32_t    num_hits  = boolop == BOOLOP_OR ? 4 : 3;
    return TestQP_new("a \"b\"", tree, NULL, num_hits);
}

static TestQueryParser*
logical_test_two_terms_one_nested(u32_t boolop)
{
    Query   *a_leaf    = make_leaf_query(NULL, "a");
    Query   *b_leaf    = make_leaf_query(NULL, "b");
    Query   *b_tree    = make_poly_query(boolop, b_leaf, NULL);
    Query   *tree      = make_poly_query(boolop, a_leaf, b_tree, NULL);
    u32_t    num_hits  = boolop == BOOLOP_OR ? 4 : 3;
    return TestQP_new("a (b)", tree, NULL, num_hits);
}

static TestQueryParser*
logical_test_one_term_one_nested_single_term_phrase(u32_t boolop)
{
    Query   *a_leaf    = make_leaf_query(NULL, "a");
    Query   *b_leaf    = make_leaf_query(NULL, "\"b\"");
    Query   *b_tree    = make_poly_query(boolop, b_leaf, NULL);
    Query   *tree      = make_poly_query(boolop, a_leaf, b_tree, NULL);
    u32_t    num_hits  = boolop == BOOLOP_OR ? 4 : 3;
    return TestQP_new("a (\"b\")", tree, NULL, num_hits);
}

static TestQueryParser*
logical_test_phrase(u32_t boolop)
{
    Query   *tree    = make_leaf_query(NULL, "\"a b\"");
    UNUSED_VAR(boolop);
    return TestQP_new("\"a b\"", tree, NULL, 3);
}

static TestQueryParser*
logical_test_nested_phrase(u32_t boolop)
{
    Query   *leaf   = make_leaf_query(NULL, "\"a b\"");
    Query   *tree   = make_poly_query(boolop, leaf, NULL);
    return TestQP_new("(\"a b\")", tree, NULL, 3);
}

static TestQueryParser*
logical_test_three_terms(u32_t boolop)
{
    Query   *a_leaf   = make_leaf_query(NULL, "a");
    Query   *b_leaf   = make_leaf_query(NULL, "b");
    Query   *c_leaf   = make_leaf_query(NULL, "c");
    Query   *tree     = make_poly_query(boolop, a_leaf, b_leaf, 
                                       c_leaf, NULL); 
    u32_t num_hits = boolop == BOOLOP_OR ? 4 : 2;
    return TestQP_new("a b c", tree, NULL, num_hits);
}

static TestQueryParser*
logical_test_three_terms_two_nested(u32_t boolop)
{
    Query   *a_leaf     = make_leaf_query(NULL, "a");
    Query   *b_leaf     = make_leaf_query(NULL, "b");
    Query   *c_leaf     = make_leaf_query(NULL, "c");
    Query   *inner_tree = make_poly_query(boolop, b_leaf, c_leaf, NULL);
    Query   *tree       = make_poly_query(boolop, a_leaf, inner_tree, NULL);
    u32_t    num_hits   = boolop == BOOLOP_OR ? 4 : 2;
    return TestQP_new("a (b c)", tree, NULL, num_hits);
}

static TestQueryParser*
logical_test_one_term_one_phrase(u32_t boolop)
{
    Query   *a_leaf   = make_leaf_query(NULL, "a");
    Query   *bc_leaf  = make_leaf_query(NULL, "\"b c\"");
    Query   *tree     = make_poly_query(boolop, a_leaf, bc_leaf, NULL);
    u32_t    num_hits = boolop == BOOLOP_OR ? 4 : 2;
    return TestQP_new("a \"b c\"", tree, NULL, num_hits);
}

static TestQueryParser*
logical_test_one_term_one_nested_phrase(u32_t boolop)
{
    Query   *a_leaf     = make_leaf_query(NULL, "a");
    Query   *bc_leaf    = make_leaf_query(NULL, "\"b c\"");
    Query   *inner_tree = make_poly_query(boolop, bc_leaf, NULL);
    Query   *tree       = make_poly_query(boolop, a_leaf, inner_tree, NULL);
    u32_t    num_hits   = boolop == BOOLOP_OR ? 4 : 2;
    return TestQP_new("a (\"b c\")", tree, NULL, num_hits);
}

static TestQueryParser*
logical_test_long_phrase(u32_t boolop)
{
    Query   *tree   = make_leaf_query(NULL, "\"a b c\"");
    UNUSED_VAR(boolop);
    return TestQP_new("\"a b c\"", tree, NULL, 2);
}

static TestQueryParser*
logical_test_pure_negation(u32_t boolop)
{
    Query   *leaf   = make_leaf_query(NULL, "x");
    Query   *tree   = make_not_query(leaf);
    UNUSED_VAR(boolop);
    return TestQP_new("-x", tree, NULL, 0);
}

static TestQueryParser*
logical_test_double_negative(u32_t boolop)
{
    Query   *tree   = make_leaf_query(NULL, "a");
    UNUSED_VAR(boolop);
    return TestQP_new("--a", tree, NULL, 4);
}

static TestQueryParser*
logical_test_triple_negative(u32_t boolop)
{
    Query   *leaf   = make_leaf_query(NULL, "a");
    Query   *tree   = make_not_query(leaf);
    UNUSED_VAR(boolop);
    return TestQP_new("---a", tree, NULL, 0);
}

/* Technically, this should produce an acceptably small result set, but it's
 * too difficult to prune -- so QParser_Prune just lops it because it's a
 * top-level NOTQuery.
 */
static TestQueryParser*
logical_test_nested_negations(u32_t boolop)
{
    Query *query = make_leaf_query(NULL, "a");
    query = make_poly_query(boolop, query, NULL);
    query = make_not_query(query);
    query = make_poly_query(BOOLOP_AND, query, NULL);
    query = make_not_query(query);
    return TestQP_new("-(-(a))", query, NULL, 0);
}

static TestQueryParser*
logical_test_two_terms_one_required(u32_t boolop)
{
    Query   *a_query = make_leaf_query(NULL, "a");
    Query   *b_query = make_leaf_query(NULL, "b");
    Query   *tree;
    if (boolop == BOOLOP_AND) {
        tree = make_poly_query(boolop, a_query, b_query, NULL);
    }
    else {
        tree = (Query*)ReqOptQuery_new(b_query, a_query);
        DECREF(b_query);
        DECREF(a_query);
    }
    return TestQP_new("a +b", tree, NULL, 3);
}

static TestQueryParser*
logical_test_intersection(u32_t boolop)
{
    Query   *a_query = make_leaf_query(NULL, "a");
    Query   *b_query = make_leaf_query(NULL, "b");
    Query   *tree    = make_poly_query(BOOLOP_AND, a_query, b_query, NULL);
    UNUSED_VAR(boolop);
    return TestQP_new("a AND b", tree, NULL, 3);
}

static TestQueryParser*
logical_test_three_way_intersection(u32_t boolop)
{
    Query *a_query = make_leaf_query(NULL, "a");
    Query *b_query = make_leaf_query(NULL, "b");
    Query *c_query = make_leaf_query(NULL, "c");
    Query *tree    = make_poly_query(BOOLOP_AND, a_query, b_query, 
                                     c_query, NULL);
    UNUSED_VAR(boolop);
    return TestQP_new("a AND b AND c", tree, NULL, 2);
}

static TestQueryParser*
logical_test_union(u32_t boolop)
{
    Query   *a_query = make_leaf_query(NULL, "a");
    Query   *b_query = make_leaf_query(NULL, "b");
    Query   *tree    = make_poly_query(BOOLOP_OR, a_query, b_query, NULL);
    UNUSED_VAR(boolop);
    return TestQP_new("a OR b", tree, NULL, 4);
}

static TestQueryParser*
logical_test_three_way_union(u32_t boolop)
{
    Query *a_query = make_leaf_query(NULL, "a");
    Query *b_query = make_leaf_query(NULL, "b");
    Query *c_query = make_leaf_query(NULL, "c");
    Query *tree = make_poly_query(BOOLOP_OR, a_query, b_query, c_query, NULL);
    UNUSED_VAR(boolop);
    return TestQP_new("a OR b OR c", tree, NULL, 4);
}

static TestQueryParser*
logical_test_a_or_plus_b(u32_t boolop)
{
    Query   *a_query = make_leaf_query(NULL, "a");
    Query   *b_query = make_leaf_query(NULL, "b");
    Query   *tree    = make_poly_query(BOOLOP_OR, a_query, b_query, NULL);
    UNUSED_VAR(boolop);
    return TestQP_new("a OR +b", tree, NULL, 4);
}

static TestQueryParser*
logical_test_and_not(u32_t boolop)
{
    Query   *a_query = make_leaf_query(NULL, "a");
    Query   *b_query = make_leaf_query(NULL, "b");
    Query   *not_b   = make_not_query(b_query);
    Query   *tree    = make_poly_query(BOOLOP_AND, a_query, not_b, NULL);
    UNUSED_VAR(boolop);
    return TestQP_new("a AND NOT b", tree, NULL, 1);
}

static TestQueryParser*
logical_test_nested_or(u32_t boolop)
{
    Query   *a_query = make_leaf_query(NULL, "a");
    Query   *b_query = make_leaf_query(NULL, "b");
    Query   *c_query = make_leaf_query(NULL, "c");
    Query   *nested  = make_poly_query(BOOLOP_OR, b_query, c_query, NULL);
    Query   *tree    = make_poly_query(boolop, a_query, nested, NULL);
    return TestQP_new("a (b OR c)", tree, NULL, boolop == BOOLOP_OR ? 4 : 3);
}

static TestQueryParser*
logical_test_and_nested_or(u32_t boolop)
{
    Query   *a_query = make_leaf_query(NULL, "a");
    Query   *b_query = make_leaf_query(NULL, "b");
    Query   *c_query = make_leaf_query(NULL, "c");
    Query   *nested  = make_poly_query(BOOLOP_OR, b_query, c_query, NULL);
    Query   *tree    = make_poly_query(BOOLOP_AND, a_query, nested, NULL);
    UNUSED_VAR(boolop);
    return TestQP_new("a AND (b OR c)", tree, NULL, 3);
}

static TestQueryParser*
logical_test_or_nested_or(u32_t boolop)
{
    Query   *a_query = make_leaf_query(NULL, "a");
    Query   *b_query = make_leaf_query(NULL, "b");
    Query   *c_query = make_leaf_query(NULL, "c");
    Query   *nested  = make_poly_query(BOOLOP_OR, b_query, c_query, NULL);
    Query   *tree    = make_poly_query(BOOLOP_OR, a_query, nested, NULL);
    UNUSED_VAR(boolop);
    return TestQP_new("a OR (b OR c)", tree, NULL, 4);
}

static TestQueryParser*
logical_test_and_not_nested_or(u32_t boolop)
{
    Query *a_query    = make_leaf_query(NULL, "a");
    Query *b_query    = make_leaf_query(NULL, "b");
    Query *c_query    = make_leaf_query(NULL, "c");
    Query *nested     = make_poly_query(BOOLOP_OR, b_query, c_query, NULL);
    Query *not_nested = make_not_query(nested);
    Query *tree       = make_poly_query(BOOLOP_AND, a_query, not_nested, NULL);
    UNUSED_VAR(boolop);
    return TestQP_new("a AND NOT (b OR c)", tree, NULL, 1);
}

static TestQueryParser*
logical_test_required_phrase_negated_term(u32_t boolop)
{
    Query *bc_query   = make_leaf_query(NULL, "\"b c\"");
    Query *d_query    = make_leaf_query(NULL, "d");
    Query *not_d      = make_not_query(d_query);
    Query *tree       = make_poly_query(BOOLOP_AND, bc_query, not_d, NULL);
    UNUSED_VAR(boolop);
    return TestQP_new("+\"b c\" -d", tree, NULL, 1);
}

static TestQueryParser*
logical_test_required_term_optional_phrase(u32_t boolop)
{
    Query *ab_query   = make_leaf_query(NULL, "\"a b\"");
    Query *d_query    = make_leaf_query(NULL, "d");
    Query *tree;
    if (boolop == BOOLOP_AND) {
        tree = make_poly_query(BOOLOP_AND, ab_query, d_query, NULL);
    }
    else {
        tree = (Query*)ReqOptQuery_new(d_query, ab_query);
        DECREF(d_query);
        DECREF(ab_query);
    }
    UNUSED_VAR(boolop);
    return TestQP_new("\"a b\" +d", tree, NULL, 1);
}

static TestQueryParser*
logical_test_nested_nest(u32_t boolop)
{
    Query *a_query    = make_leaf_query(NULL, "a");
    Query *b_query    = make_leaf_query(NULL, "b");
    Query *c_query    = make_leaf_query(NULL, "c");
    Query *d_query    = make_leaf_query(NULL, "d");
    Query *innermost  = make_poly_query(BOOLOP_AND, c_query, d_query, NULL);
    Query *inner      = make_poly_query(BOOLOP_OR, b_query, innermost, NULL);
    Query *not_inner  = make_not_query(inner);
    Query *tree       = make_poly_query(BOOLOP_AND, a_query, not_inner, NULL);
    UNUSED_VAR(boolop);
    return TestQP_new("a AND NOT (b OR (c AND d))", tree, NULL, 1);
}

static TestQueryParser*
logical_test_field_bool_group(u32_t boolop)
{
    Query   *b_query = make_leaf_query("content", "b");
    Query   *c_query = make_leaf_query("content", "c");
    Query   *tree    = make_poly_query(boolop, b_query, c_query, NULL);
    return TestQP_new("content:(b c)", tree, NULL, 
        boolop == BOOLOP_OR ? 3 : 2);
}

static TestQueryParser*
logical_test_field_multi_OR(u32_t boolop)
{
    Query *a_query = make_leaf_query("content", "a");
    Query *b_query = make_leaf_query("content", "b");
    Query *c_query = make_leaf_query("content", "c");
    Query *tree = make_poly_query(BOOLOP_OR, a_query, b_query, c_query, NULL);
    UNUSED_VAR(boolop);
    return TestQP_new("content:(a OR b OR c)", tree, NULL, 4);
}

static TestQueryParser*
logical_test_field_multi_AND(u32_t boolop)
{
    Query *a_query = make_leaf_query("content", "a");
    Query *b_query = make_leaf_query("content", "b");
    Query *c_query = make_leaf_query("content", "c");
    Query *tree    = make_poly_query(BOOLOP_AND, a_query, b_query, 
                                     c_query, NULL);
    UNUSED_VAR(boolop);
    return TestQP_new("content:(a AND b AND c)", tree, NULL, 2);
}

static TestQueryParser*
logical_test_field_phrase(u32_t boolop)
{
    Query   *tree = make_leaf_query("content", "\"b c\"");
    UNUSED_VAR(boolop);
    return TestQP_new("content:\"b c\"", tree, NULL, 2);
}

static TestQueryParser*
prune_test_null_querystring()
{
    Query   *pruned = (Query*)NoMatchQuery_new();
    return TestQP_new(NULL, NULL, pruned, 0);
}

static TestQueryParser*
prune_test_matchall()
{
    Query   *tree   = (Query*)MatchAllQuery_new();
    Query   *pruned = (Query*)NoMatchQuery_new();
    return TestQP_new(NULL, tree, pruned, 0);
}

static TestQueryParser*
prune_test_nomatch()
{
    Query   *tree   = (Query*)NoMatchQuery_new();
    Query   *pruned = (Query*)NoMatchQuery_new();
    return TestQP_new(NULL, tree, pruned, 0);
}

static TestQueryParser*
prune_test_optional_not()
{
    Query   *a_leaf  = make_leaf_query(NULL, "a");
    Query   *b_leaf  = make_leaf_query(NULL, "b");
    Query   *not_b   = make_not_query(b_leaf);
    Query   *tree    = make_poly_query(BOOLOP_OR, (Query*)INCREF(a_leaf), 
                                       not_b, NULL);
    Query   *nomatch = (Query*)NoMatchQuery_new();
    Query   *pruned  = make_poly_query(BOOLOP_OR, a_leaf, nomatch, NULL);
    return TestQP_new(NULL, tree, pruned, 4);
}

static TestQueryParser*
prune_test_reqopt_optional_not()
{
    Query   *a_leaf  = make_leaf_query(NULL, "a");
    Query   *b_leaf  = make_leaf_query(NULL, "b");
    Query   *not_b   = make_not_query(b_leaf);
    Query   *tree    = (Query*)ReqOptQuery_new(a_leaf, not_b);
    Query   *nomatch = (Query*)NoMatchQuery_new();
    Query   *pruned  = (Query*)ReqOptQuery_new(a_leaf, nomatch);
    DECREF(nomatch);
    DECREF(not_b);
    DECREF(a_leaf);
    return TestQP_new(NULL, tree, pruned, 4);
}

static TestQueryParser*
prune_test_reqopt_required_not()
{
    Query   *a_leaf  = make_leaf_query(NULL, "a");
    Query   *b_leaf  = make_leaf_query(NULL, "b");
    Query   *not_a   = make_not_query(a_leaf);
    Query   *tree    = (Query*)ReqOptQuery_new(not_a, b_leaf);
    Query   *nomatch = (Query*)NoMatchQuery_new();
    Query   *pruned  = (Query*)ReqOptQuery_new(nomatch, b_leaf);
    DECREF(nomatch);
    DECREF(not_a);
    DECREF(b_leaf);
    return TestQP_new(NULL, tree, pruned, 0);
}

static TestQueryParser*
prune_test_not_and_not()
{
    Query   *a_leaf  = make_leaf_query(NULL, "a");
    Query   *b_leaf  = make_leaf_query(NULL, "b");
    Query   *not_a   = make_not_query(a_leaf);
    Query   *not_b   = make_not_query(b_leaf);
    Query   *tree    = make_poly_query(BOOLOP_AND, not_a, not_b, NULL);
    Query   *pruned  = make_poly_query(BOOLOP_AND, NULL);
    return TestQP_new(NULL, tree, pruned, 0);
}

/***************************************************************************/

typedef TestQueryParser*
(*kino_TestQPLogic_logical_test_t)(u32_t boolop_sym);

static kino_TestQPLogic_logical_test_t logical_test_funcs[] = {
    logical_test_empty_phrase,
    logical_test_empty_parens,
    logical_test_nested_empty_parens,
    logical_test_nested_empty_phrase,
    logical_test_simple_term,
    logical_test_one_nested_term,
    logical_test_one_term_phrase,
    logical_test_two_terms,
    logical_test_two_terms_nested,
    logical_test_one_term_one_single_term_phrase,
    logical_test_two_terms_one_nested,
    logical_test_one_term_one_nested_phrase,
    logical_test_phrase,
    logical_test_nested_phrase,
    logical_test_three_terms,
    logical_test_three_terms_two_nested,
    logical_test_one_term_one_phrase,
    logical_test_one_term_one_nested_single_term_phrase,
    logical_test_long_phrase,
    logical_test_pure_negation,
    logical_test_double_negative,
    logical_test_triple_negative,
    logical_test_nested_negations,
    logical_test_two_terms_one_required,
    logical_test_intersection,
    logical_test_three_way_intersection,
    logical_test_union,
    logical_test_three_way_union,
    logical_test_a_or_plus_b,
    logical_test_and_not,
    logical_test_nested_or,
    logical_test_and_nested_or,
    logical_test_or_nested_or,
    logical_test_and_not_nested_or,
    logical_test_required_phrase_negated_term,
    logical_test_required_term_optional_phrase,
    logical_test_nested_nest,
    logical_test_field_phrase,
    logical_test_field_bool_group,
    logical_test_field_multi_OR,
    logical_test_field_multi_AND,
    NULL
};

typedef TestQueryParser*
(*kino_TestQPLogic_prune_test_t)();

static kino_TestQPLogic_prune_test_t prune_test_funcs[] = {
    prune_test_null_querystring,
    prune_test_matchall,
    prune_test_nomatch,
    prune_test_optional_not,
    prune_test_reqopt_optional_not,
    prune_test_reqopt_required_not,
    prune_test_not_and_not,
    NULL
};

static Folder*
S_create_index()
{
    Schema     *schema  = (Schema*)TestSchema_new();
    RAMFolder  *folder  = RAMFolder_new(NULL);
    VArray     *doc_set = TestUtils_doc_set();
    Indexer    *indexer = Indexer_new(schema, (Obj*)folder, NULL, NULL, 0);
    u32_t i, max;

    for (i = 0, max = VA_Get_Size(doc_set); i < max; i++) {
        static CharBuf field = ZCB_LITERAL("content");
        Doc *doc = Doc_new(NULL, 0);
        Doc_Store(doc, &field, VA_Fetch(doc_set, i));
        Indexer_Add_Doc(indexer, doc, 1.0f);
        DECREF(doc);
    }

    Indexer_Commit(indexer);

    DECREF(doc_set);
    DECREF(indexer);
    DECREF(schema);
        
    return (Folder*)folder;
}

void
TestQPLogic_run_tests()
{
    u32_t i;
    TestBatch   *batch = Test_new_batch("TestQueryParserLogic", 178, NULL);
    Folder      *folder       = S_create_index();
    Searcher    *searcher     = Searcher_new((Obj*)folder);
    QueryParser *or_parser    = QParser_new(Searcher_Get_Schema(searcher), 
        NULL, NULL, NULL);
    static  ZombieCharBuf AND = ZCB_LITERAL("AND");
    QueryParser *and_parser   = QParser_new(Searcher_Get_Schema(searcher), 
        NULL, (CharBuf*)&AND, NULL);
    QParser_Set_Heed_Colons(or_parser, true);
    QParser_Set_Heed_Colons(and_parser, true);

    PLAN(batch);

    /* Run logical tests with default boolop of OR. */
    for (i = 0; logical_test_funcs[i] != NULL; i++) {
        kino_TestQPLogic_logical_test_t test_func = logical_test_funcs[i];
        TestQueryParser *test_case = test_func(BOOLOP_OR);
        Query *tree     = QParser_Tree(or_parser, test_case->query_string);
        Query *parsed   = QParser_Parse(or_parser, test_case->query_string);
        Hits  *hits     = Searcher_Hits(searcher, (Obj*)parsed, 0, 10, NULL);

        ASSERT_TRUE(batch, Query_Equals(tree, (Obj*)test_case->tree),
            "tree() OR   %s", test_case->query_string->ptr);
        ASSERT_INT_EQ(batch, Hits_Total_Hits(hits), test_case->num_hits,
            "hits: OR   %s", test_case->query_string->ptr);
        DECREF(hits);
        DECREF(parsed);
        DECREF(tree);
        DECREF(test_case);
    }

    /* Run logical tests with default boolop of AND. */
    for (i = 0; logical_test_funcs[i] != NULL; i++) {
        kino_TestQPLogic_logical_test_t test_func = logical_test_funcs[i];
        TestQueryParser *test_case = test_func(BOOLOP_AND);
        Query *tree     = QParser_Tree(and_parser, test_case->query_string);
        Query *parsed   = QParser_Parse(and_parser, test_case->query_string);
        Hits  *hits     = Searcher_Hits(searcher, (Obj*)parsed, 0, 10, NULL);

        ASSERT_TRUE(batch, Query_Equals(tree, (Obj*)test_case->tree),
            "tree() AND   %s", test_case->query_string->ptr);
        ASSERT_INT_EQ(batch, Hits_Total_Hits(hits), test_case->num_hits,
            "hits: AND   %s", test_case->query_string->ptr);
        DECREF(hits);
        DECREF(parsed);
        DECREF(tree);
        DECREF(test_case);
    }

    /* Run tests for QParser_Prune(). */
    for (i = 0; prune_test_funcs[i] != NULL; i++) {
        kino_TestQPLogic_prune_test_t test_func = prune_test_funcs[i];
        TestQueryParser *test_case = test_func();
        CharBuf *qstring = test_case->tree 
                         ? Obj_To_String(test_case->tree)
                         : CB_new_from_trusted_utf8("(NULL)", 6);
        Query *tree = test_case->tree;
        Query *wanted = test_case->expanded;
        Query *pruned   = QParser_Prune(or_parser, tree);
        Query *expanded;
        Hits  *hits;

        ASSERT_TRUE(batch, Query_Equals(pruned, (Obj*)wanted),
            "prune()   %s", qstring->ptr);
        expanded = QParser_Expand(or_parser, pruned);
        hits = Searcher_Hits(searcher, (Obj*)expanded, 0, 10, NULL);
        ASSERT_INT_EQ(batch, Hits_Total_Hits(hits), test_case->num_hits,
            "hits:    %s", qstring->ptr);

        DECREF(hits);
        DECREF(expanded);
        DECREF(pruned);
        DECREF(qstring);
        DECREF(test_case);
    }

    DECREF(and_parser);
    DECREF(or_parser);
    DECREF(searcher);
    DECREF(folder);
    batch->destroy(batch);
}
/* Copyright 2005-2009 Marvin Humphrey
 *
 * This program is free software; you can redistribute it and/or modify
 * under the same terms as Perl itself.
 */

