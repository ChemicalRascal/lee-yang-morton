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

#define fprintf_if_eq(a, b, file, str) {if((a)==(b)){fprintf(file, str);}}

/* Currently, data is assigned to *every* node.
 */
n_qtree*
read_qtree(FILE* file, void* data)
{
    n_qtree* tree;
    int ret_val;
    unsigned int uint_read, uint_read_2;

    fprintf_if_eq(file, stdin, stdout, "Enter tree depth: ");
    ret_val = readcsv_get_uint(stdin, &uint_read);
    if (ret_val == EOF)
    {
        return NULL;
    }
    tree = new_qtree(uint_read);

    fprintf_if_eq(file, stdin, stdout,
            "\nEnter co-ords, x-coord first, EOF when complete: ");
    while ((ret_val = readcsv_get_uint(stdin, &uint_read)) != EOF)
    {
        if ((ret_val = readcsv_get_uint(stdin, &uint_read_2)) == EOF)
        {
            break;
        }
        insert_coord(tree, data, uint_read, uint_read_2, 1);
    }

    return tree;
}

int
main()
{
    n_qtree* tree;
    int dummy = 1;
    //link_node* n;

    tree = read_qtree(stdin, &dummy);
    link_nodes_morton(tree);

    print_qtree_integerwise(tree, 1);

    printf("(2,2)->(5,5): %lu\n", lee_yang(tree, 2, 2, 5, 5));

    printf("---\n");

    return 0;
}
