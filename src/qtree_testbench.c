/* vim: set et sts=4 sw=4 cc=80 tw=80: */

/*******************************************
 *
 *          File header goes here?
 *              In theory?
 *
 *******************************************/

#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <stdarg.h>

#include "bitseq.h"

#include <limits.h>

typedef struct n_qnode_s
{
    /* Base 4 'index' based on Lee-Young notation:
     *          23
     *          01
     */

    /* NOTE: For various reasons, this is easier to do as an array,
     * rather than as struct "attributes". Makes certain aspects of
     * the code simpler, but keeping the idea of which direction is
     * which is important.
     */
    struct n_qnode_s*   child[4];

    /* There should be a better way to do this, IIRC, but I haven't
     * written in C in quiiiiiiite a while. Regardless, this is used
     * if and only if the node is a leaf.
     */
    void*       data;

    /* 
     * TODO: Write a verification function that checks every node in a qtree
     *       to ensure no nodes have sub-nodes *and* a pointer to data.
     *
     * TODO: Consider writing another verification function that checks to
     *       see if all leaf nodes of a tree are on the same level.
     */
} n_qnode;

typedef struct n_qtree_s
{
    /* Canonical depth of tree, based on size given at initialisation,
     * for insertion/queries based on co-ordinates (x,y).
     *
     * Note, a depth of 0 means a null tree (a tree without any branches,
     * ergo just a leaf.) Handily, this means that a tree will have a
     * canonoical size of 2^depth by 2^depth.
     */
    unsigned int    depth;
    n_qnode*        root;
} n_qtree;

typedef struct link_node_s
{
    unsigned int            x;
    unsigned int            y;
    struct link_node_s*     n;
    void*                   data;
} link_node;

n_qnode* new_qnode(void*);
n_qtree* new_qtree(unsigned int);
link_node* new_link_node(void*, unsigned int, unsigned int);
void insert_coord(n_qtree*, void*, unsigned int, unsigned int, int);
void* query_coord(n_qtree*, unsigned int, unsigned int);
void* get_morton_lowest(n_qtree* tree);
void print_qtree_integerwise(n_qtree*, int);


void* query_coord_rec(n_qnode*, unsigned int, unsigned int,
        unsigned int, unsigned int);
void insert_coord_rec(n_qnode*, void*, unsigned int, unsigned int,
        unsigned int, unsigned int, int);
void* get_morton_lowest_rec(n_qnode* node);

n_qtree*
new_qtree(unsigned int depth)
{
    n_qtree* tree;
    tree = malloc(sizeof(n_qtree));
    assert(tree != NULL);
    tree->depth = depth;
    tree->root  = malloc(sizeof(n_qnode));
    assert(tree->root != NULL);
    tree->root = new_qnode(NULL);

    return tree;
}

n_qnode*
new_qnode(void* data)
{
    n_qnode* node;
    node = malloc(sizeof(n_qnode));
    assert(node != NULL);

    /* If there's no data, the programmer *should* have given data as NULL */
    node->data  = data;
    /* NOTE: The way the nodes work changed.
    node->sw    = NULL;
    node->se    = NULL;
    node->nw    = NULL;
    node->ne    = NULL;
     */
    node->child[0] = NULL;
    node->child[1] = NULL;
    node->child[2] = NULL;
    node->child[3] = NULL;

    return node;
}

link_node*
new_link_node(void* data, unsigned int x, unsigned int y)
{
    link_node* node;
    node = malloc(sizeof(link_node));
    assert(node != NULL);

    node->data  = data;
    node->x     = x;
    node->y     = y;
    node->n     = NULL; /* Leave the link initially as NULL. */

    return node;
}

/*TODO: Expand this to use a linked list at each leaf node. Or even
 *      an arbitary insert function that folks can pass in. Will need
 *      to also have a way to have folks pass in a deletion function
 *      to clean up data when deleting it all.
 *
 *NOTE: Currently, data just overrides existing data on a node.
 */
void
insert_coord(n_qtree* tree, void* data, unsigned int x, unsigned int y,
        int linkednodes)
{
    if (tree->root == NULL)
    {
        tree->root = new_qnode(NULL);
    }
    insert_coord_rec(tree->root, data, 0, tree->depth, x, y, linkednodes);
}

/* Recursive worker for insert_coord().
 */
void
insert_coord_rec(n_qnode* n, void* data, unsigned int d,
                unsigned int tree_depth, unsigned int x, unsigned int y,
                int linkednodes)
{
    unsigned int xbit, ybit, dir;
    xbit    = 1 & (x >> (tree_depth - d - 1));
    ybit    = 1 & (y >> (tree_depth - d - 1));
    dir     = xbit | (ybit << 1);

    if (d == tree_depth)
    {
        if (linkednodes)
        {
            n->data = new_link_node(data, x, y);
        }
        else
        {
            n->data = data;
        }
    }
    else
    {
        if (n->child[dir] == NULL)
        {
            n->child[dir] = new_qnode(NULL);
        }
        insert_coord_rec(n->child[dir], data, d+1, tree_depth, x, y,
                linkednodes);
    }
}

void
insert_coords()
{}

void*
query_coord(n_qtree* tree, unsigned int x, unsigned int y)
{
    if (tree == NULL || tree->root == NULL)
    {
        return NULL;
    }
    return query_coord_rec(tree->root, 0, tree->depth, x, y);
}

/* Recursive worker for query_coord().
 */
void*
query_coord_rec(n_qnode* n, unsigned int d, unsigned int tree_depth,
                unsigned int x, unsigned int y)
{
    unsigned int xbit, ybit, dir;
    xbit    = 1 & (x >> (tree_depth - d - 1));
    ybit    = 1 & (y >> (tree_depth - d - 1));
    dir     = xbit | (ybit << 1);

    if (d >= tree_depth)
    {
        return n->data;
    }
    else if (n->child[dir] == NULL)
    {
        return NULL;
    }
    else
    {
        return query_coord_rec(n->child[dir], d+1, tree_depth, x, y);
    }
}

void* get_morton_lowest(n_qtree* tree)
{
    if (tree == NULL || tree->root == NULL)
    {
        return NULL;
    }
    else
    {
        return get_morton_lowest_rec(tree->root);
    }
}

void* get_morton_lowest_rec(n_qnode* n)
{
    int i;
    void* p = NULL;

    if (n->data != NULL)
    {
        return n->data;
    }
    for (i = 0; i < 4; i++)
    {
        if (n->child[i] != NULL)
        {
            p = get_morton_lowest_rec(n->child[i]);
            if (p != NULL)
            {
                return p;
            }
        }
    }
    return NULL;
}

/* Counts the number of points in the rectangle defined by the pair
 * of co-ordinates (x1, y1) and (x2, y2). The search range is inclusive
 * of the boundaries those co-ordinates form.
 */
unsigned int
range_query_coord(n_qtree* tree, unsigned int x1, unsigned int y1,
                unsigned int x2, unsigned int y2)
{
    unsigned int i, j, count, lx, ly, gx, gy;

    /* Find lower/greater x, lower/greater y. */
    lx = (x1 < x2) ? x1 : x2;
    gx = (x1 > x2) ? x1 : x2;
    ly = (y1 < y2) ? y1 : y2;
    gy = (y1 > y2) ? y1 : y2;

    count = 0;
    for (i = lx; i <= gx; i++)
    {
        for (j = ly; j <= gy; j++)
        {
            if (query_coord(tree, i, j) != NULL)
            {
                count += 1;
            }
        }
    }

    return count;
}

void
print_qtree_integerwise(n_qtree* tree, int linkednodes)
{
    int i, j;
    unsigned int* point;
    void* data;

    for (i = ((1 << tree->depth) - 1); i >= 0; i--)
    {
        for (j = 0; j < (1 << tree->depth); j++)
        {
            if (linkednodes)
            {
                data = query_coord(tree, i, j);
                if (data != NULL)
                {
                    point = (unsigned int*) ((link_node*)data)->data;
                }
                else
                {
                    point = NULL;
                }
            }
            else
            {
                point = (unsigned int*) query_coord(tree, i, j);
            }
            if (point == NULL)
            {
                printf("%d ", 0);
            }
            else
            {
                printf("%d ", *point);
            }
        }
        printf("\n");
    }
}

int
main()
{
/*
    n_qtree* tree;
    int dummy = 1;
    link_node* n;

    tree = new_qtree(5);
    insert_coord(tree, &dummy, 1, 1, 1);
    insert_coord(tree, &dummy, 15, 3, 1);
    insert_coord(tree, &dummy, 30, 30, 1);
    print_qtree_integerwise(tree, 1);

    printf("0,20 to 20,0: %d\n", range_query_coord(tree, 0, 20, 20, 0));

    n = (link_node*) get_morton_lowest(tree);
    assert(n != NULL);
    printf("x: %d, y: %d\n", n->x, n->y);
*/

    bitseq* seq = new_bitseq();

    append_bit(seq, 1);
    append_bit(seq, 0);
    append_bit(seq, 1);
    append_bit(seq, 0);
    append_bit(seq, 1);
    append_bit(seq, 0);
    append_bit(seq, 0);
    append_bit(seq, 1);

    append_bit(seq, 0);
    append_bit(seq, 0);
    append_bit(seq, 1);
    append_bit(seq, 0);
    append_bit(seq, 0);
    append_bit(seq, 1);
    append_bit(seq, 1);
    append_bit(seq, 0);

    append_bit(seq, 1);
    append_bit(seq, 0);
    append_bit(seq, 1);

    pprint_bitseq(seq);


    unsigned int a = 1;
    unsigned int i;

    htobe((void*) &a, sizeof(unsigned int) * CHAR_BIT);

    for (i = 0; i < (sizeof(unsigned int)*CHAR_BIT); i++)
    {
        printf("%u", get_bit_void_ptr((void*) &a, i));
        if (i % CHAR_BIT == CHAR_BIT - 1)
        {
            printf(" ");
        }
    }
    printf("\n");

    a = 4;
    bitseq* seq2 = weave_uints(a, a);
    pprint_bitseq(seq2);

    printf("%lu\n", get_as_luint_ljust(seq2));
    printf("%lu\n", get_as_luint_rjust(seq2));
    printf("%u\n", get_as_uint_ljust(seq2));
    printf("%u\n", get_as_uint_rjust(seq2));

    bitseq* seq3 = weave_uints(4859,    0);
    bitseq* seq4 = weave_uints(4859,    74);

    pprint_bitseq(seq3);
    pprint_bitseq(seq4);

    bitseq* seq5 = weave_uints(INT_MAX  -1, 74);
    bitseq* seq6 = weave_uints(UINT_MAX -1, 74);

    pprint_bitseq(seq5);
    pprint_bitseq(seq6);

    bitseq* seq7 = new_bitseq_from_uint(555);
    printf("%u\n", get_as_uint_rjust(seq7));
    pprint_bitseq(seq7);
    return 0;
}
