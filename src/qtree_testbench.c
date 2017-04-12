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
    printf("\n");
    link_nodes_morton(tree);

    print_qtree_integerwise(tree, 1);

    printf("(2,2)->(5,5): %lu\n", lee_yang(tree, 2, 2, 5, 5));

    printf("---\n");

    qsiseq* seq = qsiseq_from_n_qtree(tree);
    pprint_qsiseq(seq);
    printf("---\n");
    printf("(2,2)->(5,5): %lu\n", lee_yang_qsi(seq, 2, 2, 5, 5));
    printf("---1\n");

    FILE* fp = fopen("psums.temp", "wb");
    write_qsipsums(seq->hi_psums, fp);
    assert(fclose(fp) == 0);

    printf("---2\n");

    free_qsipsums(seq->hi_psums);
    seq->hi_psums = NULL;
    pprint_qsiseq(seq);

    printf("---3\n");

    fp = fopen("psums.temp", "rb");
    seq->hi_psums = read_qsipsums(fp);
    assert(seq->hi_psums != NULL);
    assert(fclose(fp) == 0);
    pprint_qsiseq(seq);

    printf("---4\n");

    fp = fopen("seq_hi.temp", "wb");
    write_bitseq(seq->hi, fp);
    assert(fclose(fp) == 0);
    free_bitseq(seq->hi);
    seq->hi = NULL;
    pprint_qsiseq(seq);

    printf("---5\n");
    
    fp = fopen("seq_hi.temp", "rb");
    seq->hi = read_bitseq(fp);
    assert(seq->hi != NULL);
    assert(fclose(fp) == 0);
    pprint_qsiseq(seq);

    return 0;
}
