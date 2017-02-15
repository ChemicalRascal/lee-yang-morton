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

#define get_child_from_mcode(m, d, cd) ((m>>((cd-d-1)*2))&3)

#define node_is_leaf(n) ((n)->child[0] == NULL && n->child[1] == NULL && \
        (n)->child[2] == NULL && n->child[3] == NULL)

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

link_node* get_le(n_qtree*, unsigned int, unsigned int);
link_node* get_le_rec(n_qnode*, long unsigned int, unsigned int, unsigned int);
link_node* get_morton_highest(n_qnode*);

void link_nodes_morton(n_qtree*);
void link_nodes_morton_rec(n_qnode*, link_node**);

void* query_coord_rec(n_qnode*, unsigned int, unsigned int,
        unsigned int, unsigned int);
void insert_coord_rec(n_qnode*, void*, unsigned int, unsigned int,
        unsigned int, unsigned int, int);
void* get_morton_lowest_rec(n_qnode* node);

long unsigned int get_e_from_dp(unsigned int*, unsigned int*, unsigned int,
        unsigned int, unsigned int, unsigned int, unsigned int, unsigned int);
long unsigned int get_e_from_dp_rec(long unsigned int, unsigned int*,
        unsigned int*, unsigned int, unsigned int, unsigned int, unsigned int);

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
    int x, y;
    unsigned int* point;
    void* data;

    for (y = ((1 << tree->depth) - 1); y >= 0; y--)
    {
        for (x = 0; x < (1 << tree->depth); x++)
        {
            if (linkednodes)
            {
                data = query_coord(tree, x, y);
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
                point = (unsigned int*) query_coord(tree, x, y);
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

void
link_nodes_morton(n_qtree* tree)
{
    link_node** p;
    if (tree != NULL || tree->root != NULL)
    {
        p = malloc(sizeof(link_node*));
        assert(p != NULL);
        *p = NULL;
        link_nodes_morton_rec(tree->root, p);
        free(p);
    }
    return;
}

void
link_nodes_morton_rec(n_qnode* node, link_node** p)
{
    if (node != NULL)
    {
        if (node_is_leaf(node))
        {
            /* We're on a leaf node. That, or the tree is empty. */
            if (node->data != NULL)
            {
                ((link_node*)node->data)->n = *p;
                *p = node->data;
                printf("Found %p, linked it to %p\n", *p,
                        ((link_node*)node->data)->n);
            }
            else
            {
                /* DEBUG */
                printf("Empty tree?\n");
            }
        }

        else
        {
            /* Node has at least one child. We don't need to check for the
             * children being NULL, as the function simply does nothing if
             * it is passed NULL.
             */
            link_nodes_morton_rec(node->child[3], p);
            link_nodes_morton_rec(node->child[2], p);
            link_nodes_morton_rec(node->child[1], p);
            link_nodes_morton_rec(node->child[0], p);
            /* Recursive call in 3-2-1-0 order will mean that *p will now
             * be "lowest" (by Morton order) data point accessible from this
             * node.
             */
            node->data = *p;
            printf("Linked a branch to %p\n", node->data);
        }
    }
    return;
}

link_node*
get_le(n_qtree* tree, unsigned int x, unsigned int y)
{
    long unsigned int m;
    link_node* n;
   
    if (tree == NULL || tree->root == NULL)
    {
        return NULL;
    }
    m = weave_uints_to_luint(y, x);
    n = get_le_rec(tree->root, m, tree->depth, 0);

    if (n == NULL)
    {
        return NULL;
    }

    if (weave_uints_to_luint(n->y, n->x) < m)
    {
        /* No dp at (x, y) */
        n = n->n;
    }
    else
    {
        /* TODO: Remove, DEBUG */
        assert(weave_uints_to_luint(n->y, n->x) == m);
    }
    return n;
}

link_node*
get_le_rec(n_qnode* n, long unsigned int m, unsigned int canon_depth,
        unsigned int current_depth)
{
    int i, child;
    link_node* r = NULL;
    
    if (n == NULL)
    {
        return NULL;
    }
    if (node_is_leaf(n))
    {
        return n->data;
    }

    child = get_child_from_mcode(m, current_depth, canon_depth);

    if (n->child[child] != NULL)
    {
        r = get_le_rec(n->child[child], m, canon_depth, current_depth + 1);
        if (r != NULL)
        {
            return r;
        }
    }

    if (r == NULL || n->child[child] == NULL)
    {
        for (i = child - 1; i >= 0; i--)
        {
            r = get_morton_highest(n->child[i]);
            if (r != NULL)
            {
                return r;
            }
        }
    }

    return NULL;
}

link_node*
get_morton_highest(n_qnode* n)
{
    link_node* next;
    int i;

    if (n == NULL)
    {
        /* Terminating case. */
        return NULL;
    }
    if (node_is_leaf(n))
    {
        return n->data;
    }

    for (i = 3; i >= 0; i--)
    {
        next = get_morton_highest(n->child[i]);
        if (next != NULL)
        {
            return next;
        }
    }

    return NULL;
}


/*
 */
long unsigned int
get_e_from_dp(unsigned int* outx, unsigned int* outy,
        unsigned int dpx, unsigned int dpy,
        unsigned int lox, unsigned int loy,
        unsigned int hix, unsigned int hiy)
{
    long unsigned int sw_mcode, se_mcode, nw_mcode, ne_mcode, dp_mcode;
    unsigned int dp_region;

    unsigned int closex, closey, farx, fary;
    long unsigned int close_edge, far_edge;

    sw_mcode = weave_uints_to_luint(loy, lox);
    se_mcode = weave_uints_to_luint(loy, hix);
    nw_mcode = weave_uints_to_luint(hiy, lox);
    ne_mcode = weave_uints_to_luint(hiy, hix);
    dp_mcode = weave_uints_to_luint(dpy, dpx);

    if ((dp_mcode < sw_mcode) || (dp_mcode > ne_mcode))
    {
        /* e is undefined for dps that are fundamentally outside the mcode-low
         * and mcode-high of the query window
         *
         * Which is to say the code doesn't work for those values, but
         * Lee-Yang also doesn't need to find e for these values, so I'm just
         * treating this as undefined rather than work out what the desired
         * behaviour should be.
         */
        dp_region = 0;
    }
    else
    {
        /* Optimized search (Lee-Yang 5.4) */
        dp_region = (dpx>=lox) + 2*(dpx>hix) + 4*(dpy>=loy) + 8*(dpy>hiy);
    }
    if (dp_region == 1)
    {
        /* S */

        return get_e_from_dp_rec(dp_mcode, outx, outy, lox, loy, hix, loy);
    }
    else if (dp_region == 3)
    {
        /* SE */

        if (dp_mcode < se_mcode)
        {
            return get_e_from_dp_rec(dp_mcode, outx, outy, lox, loy, hix, loy);
        }
        else
        {
            close_edge = get_e_from_dp_rec(dp_mcode, &closex, &closey,
                    hix, loy, hix, hiy);
            far_edge = get_e_from_dp_rec(dp_mcode, &farx, &fary,
                    lox, loy, lox, hiy);
            if (far_edge < close_edge)
            {
                if (outx != NULL && outy != NULL)
                {
                    *outx = farx;
                    *outy = fary;
                }
                return far_edge;
            }
            else
            {
                if (outx != NULL && outy != NULL)
                {
                    *outx = closex;
                    *outy = closey;
                }
                return close_edge;
            }
        }
    }
    else if (dp_region == 5)
    {
        /* W */

        return get_e_from_dp_rec(dp_mcode, outx, outy, lox, loy, lox, hiy);
    }
    else if (dp_region == 7)
    {
        /* E */

        close_edge = get_e_from_dp_rec(dp_mcode, &closex, &closey,
                hix, loy, hix, hiy);
        far_edge = get_e_from_dp_rec(dp_mcode, &farx, &fary,
                lox, loy, lox, hiy);
        if (far_edge < close_edge)
        {
            if (outx != NULL && outy != NULL)
            {
                *outx = closex;
                *outy = closey;
            }
            return far_edge;
        }
        else
        {
            if (outx != NULL && outy != NULL)
            {
                *outx = closex;
                *outy = closey;
            }
            return close_edge;
        }
    }
    else if (dp_region == 12)
    {
        /* NW */

        if (dp_mcode < nw_mcode)
        {
            return get_e_from_dp_rec(dp_mcode, outx, outy, lox, loy, lox, hiy);
        }
        else
        {
            // t
            close_edge = get_e_from_dp_rec(dp_mcode, &closex, &closey,
                    lox, hiy, hix, hiy);
            // b
            far_edge = get_e_from_dp_rec(dp_mcode, &farx, &fary,
                    lox, loy, hix, loy);
            if ((far_edge < close_edge) && (dp_mcode < far_edge))
            {
                if (outx != NULL && outy != NULL)
                {
                    *outx = farx;
                    *outy = fary;
                }
                return far_edge;
            }
            else
            {
                if (outx != NULL && outy != NULL)
                {
                    *outx = closex;
                    *outy = closey;
                }
                return close_edge;
            }
        }
    }
    else if (dp_region == 13)
    {
        /* N */

        // t
        close_edge = get_e_from_dp_rec(dp_mcode, &closex, &closey,
                lox, hiy, hix, hiy);
        // b
        far_edge = get_e_from_dp_rec(dp_mcode, &farx, &fary,
                lox, loy, hix, loy);
        if ((far_edge < close_edge) && (dp_mcode < far_edge))
        {
            if (outx != NULL && outy != NULL)
            {
                *outx = closex;
                *outy = closey;
            }
            return far_edge;
        }
        else {
            if (outx != NULL && outy != NULL)
            {
                *outx = closex;
                *outy = closey;
            }
            return close_edge;
        }
    }

    printf("eh? ");
    return 0L;
}

/* Should return e_mcode. Return ULONG_MAX if dp is beyond the edge.
 *
 * TODO: Actually make it return ULONG_MAX if dp is beyond the edge. Or 0? IDK.
 */
long unsigned int
get_e_from_dp_rec(long unsigned int dp_mcode,
        unsigned int* outx, unsigned int* outy,
        unsigned int lox, unsigned int loy,
        unsigned int hix, unsigned int hiy)
{
    unsigned int mid, unmv, lo, hi;
    long unsigned int mid_mcode;
    int horiz = (loy == hiy);

    if ((loy == hiy) && (lox == hix))
    {
        if (outx != NULL && outy != NULL)
        {
            *outx = lox;
            *outy = loy;
        }
        return weave_uints_to_luint(loy, lox);
    }

    if (horiz) { unmv = loy; lo = lox; hi = hix; }
    else { unmv = lox; lo = loy; hi = hiy; }

    /* But wait! Without *this* check, the search fails on crossing bridges.
     *
     * In this case, as dp_mcode falls between the two, e is the *higher*
     * edgepoint.
     */
    if (hi == lo+1)
    {
        if (horiz)
        { return get_e_from_dp_rec(dp_mcode, outx, outy, hix, loy, hix, hiy); }
        else
        { return get_e_from_dp_rec(dp_mcode, outx, outy, lox, hiy, hix, hiy); }
    }

    mid = (lo+hi)/2;
    if (horiz) { mid_mcode = weave_uints_to_luint(unmv, mid); }
    else { mid_mcode = weave_uints_to_luint(mid, unmv); }

    if (dp_mcode < mid_mcode)
    {
        if (horiz)
        { return get_e_from_dp_rec(dp_mcode, outx, outy, lox, loy, mid, hiy); }
        else
        { return get_e_from_dp_rec(dp_mcode, outx, outy, lox, loy, hix, mid); }
    }
    else
    {
        if (horiz)
        { return get_e_from_dp_rec(dp_mcode, outx, outy, mid, loy, hix, hiy); }
        else
        { return get_e_from_dp_rec(dp_mcode, outx, outy, lox, mid, hix, hiy); }
    }
}

int
main()
{
    n_qtree* tree;
    int dummy = 1;
    //link_node* n;

    tree = new_qtree(3);
    insert_coord(tree, &dummy, 1, 0, 1);
    insert_coord(tree, &dummy, 6, 0, 1);
    insert_coord(tree, &dummy, 1, 1, 1);
    insert_coord(tree, &dummy, 2, 1, 1);
    insert_coord(tree, &dummy, 4, 1, 1);
    insert_coord(tree, &dummy, 7, 1, 1);
    insert_coord(tree, &dummy, 0, 2, 1);
    insert_coord(tree, &dummy, 3, 2, 1);
    insert_coord(tree, &dummy, 6, 2, 1);
    insert_coord(tree, &dummy, 1, 3, 1);
    insert_coord(tree, &dummy, 3, 3, 1);
    insert_coord(tree, &dummy, 5, 3, 1);
    insert_coord(tree, &dummy, 7, 3, 1);
    insert_coord(tree, &dummy, 1, 4, 1);
    insert_coord(tree, &dummy, 4, 4, 1);
    insert_coord(tree, &dummy, 6, 4, 1);
    insert_coord(tree, &dummy, 0, 5, 1);
    insert_coord(tree, &dummy, 1, 5, 1);
    insert_coord(tree, &dummy, 4, 5, 1);
    insert_coord(tree, &dummy, 2, 6, 1);
    insert_coord(tree, &dummy, 5, 6, 1);
    insert_coord(tree, &dummy, 0, 7, 1);
    insert_coord(tree, &dummy, 3, 7, 1);
    insert_coord(tree, &dummy, 7, 7, 1);

    print_qtree_integerwise(tree, 1);

    unsigned int i, j;
    for (i = 0; i <= 7; i++)
    {
        for (j = 0; j <= 7; j++)
        {
            if ((2 > i) || (i > 5) || (2 > j) || (j > 5))
            {
                printf("%u, %u: %lu -> ", j, i, weave_uints_to_luint(i, j));
                printf("%lu\n", get_e_from_dp(NULL, NULL, j, i, 2, 2, 5, 5));
            }
        }
    }


    /*
    printf("0,20 to 20,0: %d\n", range_query_coord(tree, 0, 20, 20, 0));

    n = (link_node*) get_morton_lowest(tree);
    assert(n != NULL);
    printf("x: %d, y: %d\n", n->x, n->y);
    link_nodes_morton(tree);

    n = get_morton_highest(tree->root);
    assert(n != NULL);
    printf("x: %d, y: %d, n: %p\n", n->x, n->y, n->n);

    n = get_le(tree, 17, 18);
    printf("node: %p", n);
    if (n != NULL)
    {
        printf(", x: %d, y: %d, n: %p", n->x, n->y, n->n);
    }
    printf("\n");

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
    */

    return 0;
}
