/* vim: set et sts=4 sw=4 cc=80 tw=80: */

/*******************************************
 *
 *          File header goes here?
 *              In theory?
 *
 *******************************************/

#include "leeyang.hpp"

#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <stdarg.h>
#include <limits.h>

#include "bitseq.hpp"
#include "qsi.hpp"
#include "morton.h"

#define get_child_from_mcode(m, d, cd) ((m>>((cd-d-1)*2))&3)

#define node_is_leaf(n) ((n)->child[0] == NULL && n->child[1] == NULL && \
        (n)->child[2] == NULL && n->child[3] == NULL)

n_qtree*
new_qtree(unsigned int depth)
{
    n_qtree* tree;
    tree = (n_qtree*)malloc(sizeof(n_qtree));
    assert(tree != NULL);
    tree->depth = depth;
    tree->n     = 0;
    tree->root  = (n_qnode*)malloc(sizeof(n_qnode));
    assert(tree->root != NULL);
    tree->root = new_qnode(NULL);

    return tree;
}

void
free_qtree(n_qtree* tree, int linkednodes)
{
    free_qnode(tree->root, linkednodes);
    free(tree);
    return;
}

n_qnode*
new_qnode(void* data)
{
    n_qnode* node;
    node = (n_qnode*)malloc(sizeof(n_qnode));
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

void
free_qnode(n_qnode* qnode, int linkednodes)
{
    int i;
    if (qnode == NULL)
    {
        return;
    }
    for (i = 0; i < 4; i++)
    {
        free_qnode(qnode->child[i], linkednodes);
    }
    if (linkednodes == 1)
    {
        free_link_node((link_node*)(qnode->data));
    }
    free(qnode);
    return;
}

link_node*
new_link_node(void* data, unsigned int x, unsigned int y)
{
    link_node* node;
    node = (link_node*)malloc(sizeof(link_node));
    assert(node != NULL);

    node->data  = data;
    node->x     = x;
    node->y     = y;
    node->n     = NULL; /* Leave the link initially as NULL. */

    return node;
}

void
free_link_node(link_node* n)
{
    if (n != NULL)
    {
        free(n);
    }
    return;
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
    tree->n += 1;
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

void*
get_morton_lowest(n_qtree* tree)
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

void*
get_morton_lowest_rec(n_qnode* n)
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
        p = (link_node**)malloc(sizeof(link_node*));
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
                *p = (link_node*)node->data;
            }
            else
            {
                /* DEBUG */
                fprintf(stderr, "link_nodes_morton_rec: Empty tree?\n");
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
        }
    }
    return;
}

/* Returns the data point at a given co-ordinate. If no such data point exists,
 * returns the next highest datapoint.
 */
link_node*
get_dp(n_qtree* tree, unsigned int x, unsigned int y)
{
    return get_dp_mcode(tree, weave_uints_to_luint(y, x));
}

/* Returns the data point for a given morton code. If no such data point
 * exists, returns the next highest datapoint.
 */
link_node*
get_dp_mcode(n_qtree* tree, long unsigned int m)
{
    link_node* n;
    long unsigned int n_mcode;

    n = get_dp_rec(tree->root, m, tree->depth, 0);

    if (n == NULL)
    {
        return NULL;
    }

    morton_PtoZ(n->x, n->y, &n_mcode);
    if (n_mcode < m)
    {
        /* No dp at that mcode */
        n = n->n;
    }
    else
    {
        /* TODO: Remove, DEBUG */
        assert(n_mcode == m);
    }
    return n;
}

link_node*
get_dp_rec(n_qnode* n, long unsigned int m, unsigned int canon_depth,
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
        return (link_node*)(n->data);
    }

    child = get_child_from_mcode(m, current_depth, canon_depth);

    if (n->child[child] != NULL)
    {
        r = get_dp_rec(n->child[child], m, canon_depth, current_depth + 1);
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
        return (link_node*)(n->data);
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

/* Returns the mcode of e. If something goes wrong, returns ULONG_MAX.
 *
 * outx: Pointer to where the x-coord of e should be stored.
 * outy: Pointer to where the y-coord of e should be stored.
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
        /* First, construct an integer from comparisons, and then map that
         * integer to the Lee-Yang region numbers.
         */
        dp_region = (dpx>=lox) + 2*(dpx>hix) + 4*(dpy>=loy) + 8*(dpy>hiy);
        switch(dp_region)
        {
            case 1:     /* 0001 - S  */
                dp_region = 1;
                break;
            case 3:     /* 0011 - SE */
                dp_region = 2;
                break;
            case 4:     /* 0100 - W  */
                dp_region = 6;
                break;
            case 7:     /* 0111 - E  */
                dp_region = 3;
                break;
            case 12:    /* 1100 - NW */
                dp_region = 5;
                break;
            case 13:    /* 1101 - N  */
                dp_region = 5;
                break;
            default:
                dp_region = 0;
                break;
        }
    }
    if (dp_region == 1)
    {
        /* S */

        return get_e_from_dp_rec(dp_mcode, outx, outy, lox, loy, hix, loy);
    }
    else if (dp_region == 2)
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
            if (close_edge < dp_mcode)
            {
                close_edge = ULONG_MAX;
            }
            if (far_edge < dp_mcode)
            {
                far_edge = ULONG_MAX;
            }
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
    else if (dp_region == 6)
    {
        /* W */

        return get_e_from_dp_rec(dp_mcode, outx, outy, lox, loy, lox, hiy);
    }
    else if (dp_region == 3)
    {
        /* E */

        close_edge = get_e_from_dp_rec(dp_mcode, &closex, &closey,
                hix, loy, hix, hiy);
        far_edge = get_e_from_dp_rec(dp_mcode, &farx, &fary,
                lox, loy, lox, hiy);
        if (close_edge < dp_mcode)
        {
            close_edge = ULONG_MAX;
        }
        if (far_edge < dp_mcode)
        {
            far_edge = ULONG_MAX;
        }
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
    else if (dp_region == 5)
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
            if (close_edge < dp_mcode)
            {
                close_edge = ULONG_MAX;
            }
            if (far_edge < dp_mcode)
            {
                far_edge = ULONG_MAX;
            }
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
    else if (dp_region == 4)
    {
        /* N */

        // t
        close_edge = get_e_from_dp_rec(dp_mcode, &closex, &closey,
                lox, hiy, hix, hiy);
        // b
        far_edge = get_e_from_dp_rec(dp_mcode, &farx, &fary,
                lox, loy, hix, loy);
        if (close_edge < dp_mcode)
        {
            close_edge = ULONG_MAX;
        }
        if (far_edge < dp_mcode)
        {
            far_edge = ULONG_MAX;
        }
        if (far_edge < close_edge)
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

    /* Something is horribly wrong. */
    return ULONG_MAX;
}

/* Returns e_mcode. Returns ULONG_MAX if dp is beyond the edge.
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

/* Warning: Here be excessive function parameters.
 */
long unsigned int
get_fp_from_dp_e(
        unsigned int* outx, unsigned int* outy,
        long unsigned int dp_mcode, long unsigned int e_mcode,
        unsigned int lox, unsigned int loy,
        unsigned int hix, unsigned int hiy,
        unsigned int tree_depth)
{
    long unsigned int fp_mcode = 0;
    long unsigned int dp_digit, e_digit, i, e_x, e_y, dpx, dpy;

    morton_ZtoP(e_mcode, &e_x, &e_y);
    morton_ZtoP(dp_mcode, &dpx, &dpy);

    for (i = 0; i < tree_depth; i++)
    {
        dp_digit = (dp_mcode >> ((tree_depth-i-1)*2) & 3);
        e_digit = (e_mcode >> ((tree_depth-i-1)*2) & 3);
        if (e_digit == dp_digit)
        {
            fp_mcode += e_digit;
            fp_mcode <<= 2;
        }
        else
        {
            if ((e_digit - dp_digit) != 1)
            {
                fp_mcode += (e_digit - 1);
                i++;
            }
            else 
            {
                assert((e_digit - dp_digit) == 1);
                fp_mcode += e_digit;
                i++;
            }
            break;
        }
    }
    if (tree_depth > i)
    {
        fp_mcode <<= (tree_depth - i)*2;
    }
    return fp_mcode;
}

/* Returns the mcode of fp.
 *
 * dpx, dpy:    Co-ords of dp.
 * lox, loy:    Co-ords of query SW corner.
 * hix, hiy:    Co-ords of query NE corner.
 * tree_depth:  Canonical depth of qtree.
 */
long unsigned int
get_fp_from_dp(unsigned int dpx, unsigned int dpy,
        unsigned int lox, unsigned int loy,
        unsigned int hix, unsigned int hiy,
        unsigned int tree_depth)
{
    long unsigned int mcode, dp_mcode;

    dp_mcode = weave_uints_to_luint(dpy, dpx);

    mcode = get_e_from_dp(NULL, NULL, dpx, dpy, lox, loy, hix, hiy);
    mcode = get_fp_from_dp_e(NULL, NULL, dp_mcode, mcode, lox, loy, hix, hiy,
            tree_depth);

    return mcode;
}

/* Performs a Lee-Yang, Morton-path based range query on the window defined
 * by (lox, loy) and (hix, hiy). Returns the number of data points in the
 * window.
 *
 * tree:    Pointer to n_qtree to perform the range query upon. Must have been
 *          initialised properly, and must use link_nodes.
 * lox:     x-coord of SW corner of query window
 * loy:     y-coord of SW corner of query window
 * hix:     x-coord of NE corner of query window
 * hiy:     y-coord of NE corner of query window
 */
long unsigned int
lee_yang(n_qtree* tree, unsigned int lox, unsigned int loy, unsigned int hix,
        unsigned int hiy)
{
    long unsigned int le_mcode, ge_mcode, count = 0L;
    link_node* n;

    if ((tree == NULL) || (tree->root == NULL))
    {
        return 0L;
    }

    le_mcode = weave_uints_to_luint(loy, lox);
    ge_mcode = weave_uints_to_luint(hiy, hix);

    n = get_dp_mcode(tree, le_mcode);

    while ((n != NULL) && (weave_uints_to_luint(n->y, n->x) <= ge_mcode))
    {
        if ((lox <= n->x) && (n->x <= hix) && (loy <= n->y) && (n->y <= hiy))
        {
            /* We're in an internal run. */
            count += 1;
            n = n->n;
            continue;
        }
        else
        {
            /* We're in an external run. Skip it -- not nessecarily *to* an
             * internal run, mind, but at a minimum we can go to the next dp
             * that is after *this* external run.
             */
            n = get_dp_mcode(tree, get_fp_from_dp(n->x, n->y, lox, loy, hix,
                        hiy, tree->depth));
        }
    }

    return count;
}

qsiseq*
qsiseq_from_n_qtree(n_qtree* tree)
{
    link_node* n;
    qsiseq* seq = new_qsiseq();

    seq->tree_depth = tree->depth;
    qsi_set_u(seq, (1<<((tree->depth)*2)));
    n = get_morton_highest(tree->root);
    qsi_set_n(seq, tree->n);

    n = (link_node*)get_morton_lowest(tree);
    while (n != NULL)
    {
        qsi_append(seq, weave_uints_to_luint(n->y, n->x));
        n = n->n;
    }
    qsi_update_psums(seq);

    return seq;
}

/* Performs a Lee-Yang, Morton-path based range query on the window defined
 * by (lox, loy) and (hix, hiy). Returns the number of data points in the
 * window.
 *
 * seq:     Pointer to qsiseq to perform the range query upon. Generate via
 *          qsiseq_from_n_qtree().
 * lox:     x-coord of SW corner of query window
 * loy:     y-coord of SW corner of query window
 * hix:     x-coord of NE corner of query window
 * hiy:     y-coord of NE corner of query window
 */
long unsigned int
lee_yang_qsi(qsiseq* seq, unsigned int lox, unsigned int loy, unsigned int hix,
        unsigned int hiy)
{
    long unsigned int le_mcode, ge_mcode, n, count = 0L;
    long unsigned int w_x, w_y;
    qsi_next_state seq_state;

    if (seq == NULL)
    {
        return 0L;
    }

    le_mcode = weave_uints_to_luint(loy, lox);
    ge_mcode = weave_uints_to_luint(hiy, hix);

    n = qsi_get(seq, &seq_state, le_mcode);

    while ((n != ULONG_MAX) && (n <= ge_mcode))
    {
        morton_ZtoP(n, &w_x, &w_y);
        if ((lox <= w_x) && (w_x <= hix) && (loy <= w_y) && (w_y <= hiy))
        {
            /* We're in an internal run. */
            count += 1;
            n = qsi_get_next(seq, &seq_state);
            continue;
        }
        else
        {
            /* We're in an external run. Skip it -- not nessecarily *to* an
             * internal run, mind, but at a minimum we can go to the next dp
             * that is after *this* external run.
             */
            n = get_fp_from_dp(w_x, w_y, lox, loy, hix, hiy, seq->tree_depth);
            n = qsi_get(seq, &seq_state, n);
        }
    }

    return count;
}

/* ---------------------------------------------------------------------------
 *                   morton.h-enhanced functions
 *
 * (Also only calculates the corner mcodes once.)
 * ---------------------------------------------------------------------------
 */

/* Returns e_mcode. Returns ULONG_MAX if dp is beyond the edge. Uses morton.h.
 *
 * dp_mcode:    mcode of datapoint (external run) being used to find the next
 *              internal run.
 * outx:        pointer to place to store e_x
 * outy:        pointer to place to store e_y
 * lox:         low x value of edge being considered
 * loy:         low y value of edge being considered
 * hix:         high x value of edge being considered
 * hiy:         high y value of edge being considered
 *
 *
 * TODO: Actually make it return ULONG_MAX if dp is beyond the edge. Or 0? IDK.
 */
uint64_t
fast_get_e_from_dp_rec(long unsigned int dp_mcode,
        unsigned int* outx, unsigned int* outy,
        unsigned int lox, unsigned int loy,
        unsigned int hix, unsigned int hiy)
{
    uint64_t r, mid_mcode;
    unsigned int mid, unmv, lo, hi;
    int horiz = (loy == hiy);

    if ((loy == hiy) && (lox == hix))
    {
        if (outx != NULL && outy != NULL)
        {
            *outx = lox;
            *outy = loy;
        }
        morton_PtoZ(lox, loy, &r);
        return r;
    }

    if (horiz)
    {
        unmv = loy;
        lo = lox;
        hi = hix;
    }
    else
    {
        unmv = lox;
        lo = loy;
        hi = hiy;
    }

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
    if (horiz)
    {
        morton_PtoZ(mid, unmv, &mid_mcode);
    }
    else
    {
        morton_PtoZ(unmv, mid, &mid_mcode);
    }

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

/* Warning: Here be excessive function parameters.
 */
long unsigned int
fast_get_fp_from_dp_e(
        unsigned int* outx, unsigned int* outy,
        unsigned int dpx, unsigned int dpy,
        uint64_t dp_mcode, uint64_t e_mcode,
        unsigned int lox, unsigned int loy,
        unsigned int hix, unsigned int hiy,
        unsigned int tree_depth)
{
    long unsigned int fp_mcode = 0;
    unsigned int dp_digit, e_digit, i;
    uint64_t e_x, e_y;

    morton_ZtoP(e_mcode, &e_y, &e_x);
    for (i = 0; i < tree_depth; i++)
    {
        dp_digit = (dp_mcode >> ((tree_depth-i-1)*2) & 3);
        e_digit = (e_mcode >> ((tree_depth-i-1)*2) & 3);
        if (e_digit == dp_digit)
        {
            fp_mcode += e_digit;
            fp_mcode <<= 2;
        }
        else
        {
            if ((e_digit - dp_digit) != 1)
            {
                fp_mcode += (e_digit - 1);
                i++;
            }
            else
            {
                assert((e_digit - dp_digit) == 1);
                fp_mcode += e_digit;
                i++;
            }
            break;
        }
    }
    if (tree_depth > i)
    {
        fp_mcode <<= (tree_depth - i)*2;
    }
    return fp_mcode;
}

/* Returns the mcode of e. If something goes wrong, returns ULONG_MAX. Uses
 * morton.h.
 *
 * outx: Pointer to where the x-coord of e should be stored.
 * outy: Pointer to where the y-coord of e should be stored.
 */
long unsigned int
fast_get_e_from_dp(unsigned int* outx, unsigned int* outy,
        unsigned int dpx, unsigned int dpy, uint64_t dp_mcode,
        unsigned int lox, unsigned int loy,
        unsigned int hix, unsigned int hiy,
        uint64_t sw_mcode, uint64_t se_mcode,
        uint64_t nw_mcode, uint64_t ne_mcode)
{
    unsigned int dp_region;
    unsigned int closex, closey, farx, fary;
    long unsigned int close_edge, far_edge;

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
        /* First, construct an integer from comparisons, and then map that
         * integer to the Lee-Yang region numbers.
         */
        dp_region = (dpx>=lox) + 2*(dpx>hix) + 4*(dpy>=loy) + 8*(dpy>hiy);
        switch(dp_region)
        {
            case 1:     /* 0001 - S  */
                dp_region = 1;
                break;
            case 3:     /* 0011 - SE */
                dp_region = 2;
                break;
            case 4:     /* 0100 - W  */
                dp_region = 6;
                break;
            case 7:     /* 0111 - E  */
                dp_region = 3;
                break;
            case 12:    /* 1100 - NW */
                dp_region = 5;
                break;
            case 13:    /* 1101 - N  */
                dp_region = 5;
                break;
            default:
                dp_region = 0;
                break;
        }
    }
    if (dp_region == 1)
    {
        /* S */

        return get_e_from_dp_rec(dp_mcode, outx, outy, lox, loy, hix, loy);
    }
    else if (dp_region == 2)
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
            if (close_edge < dp_mcode)
            {
                close_edge = ULONG_MAX;
            }
            if (far_edge < dp_mcode)
            {
                far_edge = ULONG_MAX;
            }
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
    else if (dp_region == 6)
    {
        /* W */

        return get_e_from_dp_rec(dp_mcode, outx, outy, lox, loy, lox, hiy);
    }
    else if (dp_region == 3)
    {
        /* E */

        close_edge = get_e_from_dp_rec(dp_mcode, &closex, &closey,
                hix, loy, hix, hiy);
        far_edge = get_e_from_dp_rec(dp_mcode, &farx, &fary,
                lox, loy, lox, hiy);
        if (close_edge < dp_mcode)
        {
            close_edge = ULONG_MAX;
        }
        if (far_edge < dp_mcode)
        {
            far_edge = ULONG_MAX;
        }
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
    else if (dp_region == 5)
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
            if (close_edge < dp_mcode)
            {
                close_edge = ULONG_MAX;
            }
            if (far_edge < dp_mcode)
            {
                far_edge = ULONG_MAX;
            }
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
    else if (dp_region == 4)
    {
        /* N */

        // t
        close_edge = get_e_from_dp_rec(dp_mcode, &closex, &closey,
                lox, hiy, hix, hiy);
        // b
        far_edge = get_e_from_dp_rec(dp_mcode, &farx, &fary,
                lox, loy, hix, loy);
        if (close_edge < dp_mcode)
        {
            close_edge = ULONG_MAX;
        }
        if (far_edge < dp_mcode)
        {
            far_edge = ULONG_MAX;
        }
        if (far_edge < close_edge)
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

    /* Something is horribly wrong. */
    return ULONG_MAX;
}

/* Returns the mcode of fp. Uses morton.h.
 *
 * dpx, dpy:    Co-ords of dp.
 * lox, loy:    Co-ords of query SW corner.
 * hix, hiy:    Co-ords of query NE corner.
 * tree_depth:  Canonical depth of qtree.
 */
long unsigned int
fast_get_fp_from_dp(unsigned int dpx, unsigned int dpy, uint64_t dp_mcode,
        unsigned int lox, unsigned int loy,
        unsigned int hix, unsigned int hiy,
        uint64_t sw_mcode, uint64_t se_mcode,
        uint64_t nw_mcode, uint64_t ne_mcode,
        unsigned int tree_depth)
{
    uint64_t mcode;
    //morton_PtoZ(dpx, dpy, &dp_mcode);
    mcode = fast_get_e_from_dp(NULL, NULL, dpx, dpy, dp_mcode,
            lox, loy, hix, hiy, sw_mcode, se_mcode, nw_mcode, ne_mcode);
    mcode = fast_get_fp_from_dp_e(NULL, NULL, dpx, dpy, dp_mcode, mcode,
            lox, loy, hix, hiy, tree_depth);

    return mcode;
}

/* Performs a Lee-Yang, Morton-path based range query on the window defined
 * by (lox, loy) and (hix, hiy). Returns the number of data points in the
 * window. Uses morton.h
 *
 * seq:     Pointer to qsiseq to perform the range query upon. Generate via
 *          qsiseq_from_n_qtree().
 * lox:     x-coord of SW corner of query window
 * loy:     y-coord of SW corner of query window
 * hix:     x-coord of NE corner of query window
 * hiy:     y-coord of NE corner of query window
 */
long unsigned int
fast_lee_yang_qsi(qsiseq* seq, unsigned int lox, unsigned int loy,
        unsigned int hix, unsigned int hiy)
{
    uint64_t sw_mcode, se_mcode, nw_mcode, ne_mcode, n, w_x, w_y;
    long unsigned int count = 0L;
    qsi_next_state seq_state;

    if (seq == NULL)
    {
        return 0L;
    }

    morton_PtoZ(lox, loy, &sw_mcode);
    morton_PtoZ(hix, loy, &se_mcode);
    morton_PtoZ(lox, hiy, &nw_mcode);
    morton_PtoZ(hix, hiy, &ne_mcode);

    n = qsi_get(seq, &seq_state, sw_mcode);

    while ((n != ULONG_MAX) && (n <= ne_mcode))
    {
        morton_ZtoP(n, &w_x, &w_y);
        if ((lox <= w_x) && (w_x <= hix) && (loy <= w_y) && (w_y <= hiy))
        {
            /* We're in an internal run. */
            count += 1;
            n = qsi_get_next(seq, &seq_state);
            continue;
        }
        else
        {
            /* We're in an external run. Skip it -- not nessecarily *to* an
             * internal run, mind, but at a minimum we can go to the next dp
             * that is after *this* external run.
             */
            n = fast_get_fp_from_dp(w_x, w_y, n, lox, loy, hix, hiy,
                    sw_mcode, se_mcode, nw_mcode, ne_mcode, seq->tree_depth);
            n = qsi_get(seq, &seq_state, n);
        }
    }

    return count;
}
