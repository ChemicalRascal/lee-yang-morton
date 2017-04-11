/* vim: set et sts=4 sw=4 cc=80 tw=80: */

/*******************************************
 *
 *  Main testbench program for lee_yang.h,
 *      and associated libraries.
 *
 *******************************************/

#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <stdarg.h>

#include "bitseq.h"
#include "qsi.h"
#include "leeyang.h"
#include "read_csv.h"

#include <limits.h>

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

    link_nodes_morton(tree);

    print_qtree_integerwise(tree, 1);

    printf("(2,2)->(5,5): %lu\n", lee_yang(tree, 2, 2, 5, 5));

    unsigned int i, j;
    long unsigned int dp, e, fp;
    for (i = 0; i <= 7; i++)
    {
        for (j = 0; j <= 7; j++)
        {
            /*
            dp = weave_uints_to_luint(i, j);
            printf("%u, %u: %lu -> ", j, i, dp);
            unweave_luint_to_uints(dp, &y, &x);
            printf("%u, %u\n", x, y);
            */
            if ((2 > i) || (i > 5) || (2 > j) || (j > 5))
            {
                unsigned int e_x, e_y;
                dp = weave_uints_to_luint(i, j);
                e = get_e_from_dp(&e_x, &e_y, j, i, 2, 2, 5, 5);
                fp = get_fp_from_dp_e(NULL, NULL, dp, e, 2, 2, 5, 5,
                        tree->depth);
                assert(fp == get_fp_from_dp(j, i, 2, 2, 5, 5, tree->depth));

                if (e != 0)
                {
                    printf("%u, %u: %lu -> ", j, i, dp);
                    printf("%lu -> ", e);
                    printf("%lu\n", fp);
                }
            }
        }
    }

    printf("---\n");

    qsiseq* qsiseq = new_qsiseq();
    qsi_set_u(qsiseq, 32+32+32+32);
    qsi_set_n(qsiseq, 20);

    qsi_append(qsiseq, 5);
    qsi_append(qsiseq, 8);
    qsi_append(qsiseq, 8);
    qsi_append(qsiseq, 15);
    qsi_append(qsiseq, 32);
    qsi_update_psums(qsiseq);
    qsi_append(qsiseq, 32+5);
    qsi_append(qsiseq, 32+8);
    qsi_append(qsiseq, 32+8);
    qsi_append(qsiseq, 32+15);
    qsi_append(qsiseq, 32+32);
    qsi_update_psums(qsiseq);
    qsi_append(qsiseq, 32+32+5);
    qsi_append(qsiseq, 32+32+8);
    qsi_append(qsiseq, 32+32+8);
    qsi_append(qsiseq, 32+32+15);
    qsi_append(qsiseq, 32+32+32);
    qsi_update_psums(qsiseq);
    qsi_append(qsiseq, 32+32+32+5);
    qsi_append(qsiseq, 32+32+32+8);
    qsi_append(qsiseq, 32+32+32+8);
    qsi_append(qsiseq, 32+32+32+15);
    qsi_append(qsiseq, 32+32+32+32);
    qsi_update_psums(qsiseq);
    pprint_qsiseq(qsiseq);

    printf("max: %lu\n", qsiseq->max);

    qsi_next_state ns;
    i = 0;
    ns.lo = ns.hi = ns.running_psum = 0L;
    printf("%3lu, %3lu: ", ns.hi, ns.lo);
    while ((dp = qsi_get_next(qsiseq, &ns)) != ULONG_MAX)
    {
        printf("%3u: %3lu.\n", i++, dp);
        printf("%3lu, %3lu: ", ns.hi, ns.lo);
    }
    printf("\n");

    ns.lo = ns.hi = ns.running_psum = 0L;
    for (i = 0; i < (qsiseq->max + 1); i++)
    {
        printf("%3u: %3lu -- ", i, qsi_get(qsiseq, &ns, i));
        printf("%3lu, %3lu\n", ns.hi, ns.lo);
    }

    printf("---\n");
    long unsigned int read_val;
    while (readcsv_get_luint(stdin, &read_val) != EOF)
    {
        printf("%lu was entered\n", read_val);
    }

    return 0;
}
