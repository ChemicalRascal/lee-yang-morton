/* vim: set et sts=4 sw=4 cc=80 tw=80: */

/*******************************************
 *
 *  Main testbench program for leeyang.h,
 *      and associated libraries.
 *
 *******************************************/

#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <stdarg.h>

#include "bitseq.hpp"
#include "qsi.hpp"
#include "leeyang.hpp"
#include "read_csv.h"
#include "morton.h"
#include "bit_qtree.hpp"

#include <limits.h>

#include <unistd.h>
#include <string.h>
#include <sys/time.h>

#include <sdsl/vectors.hpp>

#define fprintf_if_eq(a, b, fp, args...) {if((a)==(b)){fprintf((fp), args);}}
#define q_fprintf(fp, args...) fprintf_if_eq(global_quiet_mode, 0, fp, args)
#define q_fprintf_if_eq(a, b, fp, args...) {if((a)==(b))\
    {q_fprintf((fp), args);}}

int global_quiet_mode;
char* global_input_path;
char* global_tree_path;

/* Returns EOF if things went badly.
 */
int
read_coord(FILE* fp, unsigned int* x, unsigned int* y)
{
    unsigned int uint_read, uint_read_2;
    if (readcsv_get_uint(fp, &uint_read) != EOF)
    {
        if (readcsv_get_uint(fp, &uint_read_2) != EOF)
        {
            *x = uint_read;
            *y = uint_read_2;
            return 0;
        }
    }
    return EOF;
}

/* Returns EOF if things went badly.
 */
int
read_query_range(FILE* fp, unsigned int* lox, unsigned int* loy,
        unsigned int* hix, unsigned int* hiy)
{
    unsigned int x1, x2, y1, y2;

    if (read_coord(fp, &x1, &y1) != EOF)
    {
        if (read_coord(fp, &x2, &y2) != EOF)
        {
            *lox = x1 < x2 ? x1 : x2;
            *loy = y1 < y2 ? y1 : y2;
            *hix = x1 > x2 ? x1 : x2;
            *hiy = y1 > y2 ? y1 : y2;
            return 0;
        }
    }
    return EOF;
}

/* Currently, data is assigned to *every* node.
 */
n_qtree*
read_qtree(FILE* fp, void* data)
{
    n_qtree* tree;
    unsigned int x, y;

    q_fprintf_if_eq(global_input_path, NULL, stdout, "Enter tree depth: ");
    if (readcsv_get_uint(fp, &x) == EOF)
    {
        return NULL;
    }
    tree = new_qtree(x);

    q_fprintf_if_eq(global_input_path, NULL, stdout,
            "\nEnter co-ords, x-coord first, EOF when complete: ");
    while (read_coord(fp, &x, &y) != EOF)
    {
        insert_coord(tree, data, x, y, 1);
    }

    return tree;
}

void
exit_testbed(char** argv)
{
    int i, j;
    long unsigned int z;
    long unsigned int xa, ya;
    long unsigned int xb, yb;
    for (j = 7; j >= 0; j--)
    {
        for (i = 0; i < 8; i++)
        {
            morton_PtoZ(i, j, &z);
            printf("%2lu ", z);
        }
        printf("\n");
    }

    for (z = 0; z > 4096; z++)
    {
        morton_ZtoP(z, &xa, &ya);
        morton_ZtoP(z, &xb, &yb);
        if ((xa != xb) || (ya != yb))
        {
            printf("%lu\n", z);
        }
    }

    exit(EXIT_SUCCESS);
}

void
exit_fprintf_help(char** argv)
{
    fprintf(stdout, "Usage: %s [OPTION]... -t FILE\n", argv[0]);
    fprintf(stdout, "Perform range queries using the Lee-Yang algorithm.\n");
    fprintf(stdout, "\n");
    fprintf(stdout, "  -t FILE must be provided.\n");
    fprintf(stdout, "\n");
    fprintf(stdout, "  -b           build 'treefile'\n");
    fprintf(stdout, "  -q           make no output to stdout\n");
    fprintf(stdout, "  -c           do timing comparisons instead\n");
    fprintf(stdout, "  -f FILE      read from FILE instead of stdin\n");
    fprintf(stdout, "  -t FILE      use FILE as 'treefile'\n");
    exit(EXIT_SUCCESS);
}

void
exit_fprintf_usage(char** argv)
{
    /*
     */
    fprintf(stderr, "Usage: %s [OPTION]... -t [FILE]\n", argv[0]);
    fprintf(stderr, "Try '%s --help' for more information.\n", argv[0]);
    exit(EXIT_FAILURE);
}

int
main(int argc, char** argv, char** envp)
{
    int opt, build_mode, print_mode, timing_mode;
    FILE* input_fp;
    std::fstream tree_file;
    qsiseq* seq;

    if (argc == 1)
    {
        exit_fprintf_usage(argv);
    }
    if (argc == 2)
    {
        if (strcmp(argv[1], "--help") == 0)
        {
            exit_fprintf_help(argv);
        }

        if (strcmp(argv[1], "--test") == 0)
        {
            exit_testbed(argv);
        }
    }

    global_quiet_mode = 0;
    build_mode = 0;
    print_mode = 0;
    timing_mode = 0;
    global_input_path = NULL;
    global_tree_path = NULL;
    input_fp = NULL;

    while ((opt = getopt(argc, argv, "bqpcf:t:")) != -1)
    {
        switch (opt)
        {
            case 'b':
                build_mode = 1;
                break;
            case 'q':
                global_quiet_mode = 1;
                break;
            case 'p':
                print_mode = 1;
                break;
            case 'f':
                global_input_path = optarg;
                break;
            case 't':
                global_tree_path = optarg;
                break;
            case 'c':
                timing_mode = 1;
                break;
            default:
                exit_fprintf_usage(argv);
                break;
        }
    }

    if (global_tree_path == NULL)
    {
        exit_fprintf_usage(argv);
    }
    else
    {
        tree_file = std::fstream(global_tree_path, std::fstream::binary |
                ((build_mode == 1)
                    ? (std::fstream::out | std::fstream::trunc)
                    : (std::fstream::in))
                );
    }

    if (global_input_path == NULL)
    {
        input_fp = stdin;
    }
    else
    {
        input_fp = fopen(global_input_path, "r");
    }

    if (build_mode == 1)
    {
        n_qtree* tree;
        BitQTree bitqtree;
        int junk_data = 1;

        tree = read_qtree(input_fp, &junk_data);
        link_nodes_morton(tree);
        if (print_mode == 1)
        {
            print_qtree_integerwise(tree, 0);
        }
        seq = qsiseq_from_n_qtree(tree);
        write_qsiseq(seq, tree_file);
        if (print_mode == 1)
        {
            pprint_qsiseq(seq);
        }

        free_qsiseq(seq);
        tree_file.close();
        tree_file = std::fstream("bit_qtree_file", std::fstream::binary |
                    std::fstream::out | std::fstream::trunc);
        bitqtree = BitQTree(tree);
        bitqtree.serialize(tree_file);

        free_qtree(tree, 1);
        bitqtree = BitQTree();
        tree_file = std::fstream("bit_qtree_file", std::fstream::binary |
                    std::fstream::in);
        bitqtree.load(tree_file);
        tree = bitqtree.remake_tree();
        if (print_mode == 1)
        {
            print_qtree_integerwise(tree, 0);
        }
        exit(EXIT_SUCCESS);
    }

    if (build_mode == 0)
    {
        long int slow_ly, fast_ly, slow_time, fast_time;
        long int time_diff;
        struct timeval flag1, flag2, flag3;
        unsigned int lox, loy, hix, hiy;
        lox = loy = hix = hiy = 0;

        seq = read_qsiseq(tree_file);
        if (print_mode == 1)
        {
            pprint_qsiseq(seq);
        }
        while (read_query_range(input_fp, &lox, &loy, &hix, &hiy) != EOF)
        {
            if (timing_mode == 1)
            {
                printf("%3u,%3u %3u,%3u: ", lox, loy, hix, hiy);

                gettimeofday(&flag1, NULL);
                slow_ly = lee_yang_qsi(seq, lox, loy, hix, hiy);
                gettimeofday(&flag2, NULL);
                fast_ly = fast_lee_yang_qsi(seq, lox, loy, hix, hiy);
                gettimeofday(&flag3, NULL);

                slow_time = flag2.tv_usec - flag1.tv_usec;
                if (slow_time < 0)
                {
                    slow_time += 1000000;
                }
                fast_time = flag3.tv_usec - flag2.tv_usec;
                if (fast_time < 0)
                {
                    fast_time += 1000000;
                }
                time_diff = fast_time - slow_time;

                printf("%3ld, %3ld ", slow_ly, fast_ly);
                assert(slow_ly == fast_ly);
                printf("slow: %4ld, fast: %4ld, diff: %6ld\n", slow_time,
                        fast_time, time_diff);
            }
            else
            {
                printf("%u,%u %u,%u: ", lox, loy, hix, hiy);
                printf("%lu\n", fast_lee_yang_qsi(seq, lox, loy, hix, hiy));
            }
        }
    }

    exit(EXIT_SUCCESS);
}
