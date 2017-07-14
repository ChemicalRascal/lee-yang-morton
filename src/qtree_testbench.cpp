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
#include <list>
#include <tuple>

#include "bitseq.hpp"
#include "qsi.hpp"
#include "leeyang.hpp"
#include "read_csv.h"
#include "morton.h"
#include "bit_qtree.hpp"

#include "offset_qtree.hpp"

#include <limits.h>

#include <unistd.h>
#include <string.h>
#include <sys/time.h>

#include <sdsl/vectors.hpp>

#define fprintf_if_eq(a, b, fp, args...) {if((a)==(b)){fprintf((fp), args);}}
#define q_fprintf(fp, args...) fprintf_if_eq(global_quiet_mode, 0, fp, args)
#define q_fprintf_if_eq(a, b, fp, args...) {if((a)==(b))\
    {q_fprintf((fp), args);}}

int     global_quiet_mode;
char*   global_prefix_arg;

enum opmode_t
{
    null_mode,   //
    qsi_mode,    // -c
    bqt_mode,    // -d
    oqt_mode,    // -e
};

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
//TODO: Move this to std::istream
n_qtree*
read_qtree(FILE* fp, void* data)
{
    n_qtree* tree;
    unsigned int x, y;

    q_fprintf(stdout, "Enter tree depth: ");
    if (readcsv_get_uint(fp, &x) == EOF)
    {
        return NULL;
    }
    tree = new_qtree(x);

    q_fprintf(stdout, "\nEnter co-ords, x-coord first, EOF when complete: ");
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
    //FIXME: Update this
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
    //FIXME: Update this
    fprintf(stderr, "Usage: %s [OPTION]... -t [FILE]\n", argv[0]);
    fprintf(stderr, "Try '%s --help' for more information.\n", argv[0]);
    exit(EXIT_FAILURE);
}

int
main(int argc, char** argv, char** envp)
{
    int opt, build_mode, print_mode, timing_mode;
    FILE* input_fp;

    std::list<std::tuple<opmode_t, unsigned int>> mode_l =
        std::list<std::tuple<opmode_t, unsigned int>>();

    std::string prefix;
    //FIXME: Currently unused
    //std::fstream csv_file;
    std::fstream tree_file;

    // vars used in various modes
    qsiseq* qsiseq;
    BitQTree bitqtree;
    OffsetQTree<unsigned int> oqt;

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
    global_prefix_arg = NULL;
    input_fp = NULL;

    while ((opt = getopt(argc, argv, "bqpx:tc:d:e:")) != -1)
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
            case 'x':
                global_prefix_arg = optarg;
                break;
            case 't':
                timing_mode = 1;
                break;
            case 'c':
                mode_l.push_back(std::tuple<opmode_t, unsigned int>
                        (qsi_mode, atoi(optarg)));
                break;
            case 'd':
                mode_l.push_back(std::tuple<opmode_t, unsigned int>
                        (bqt_mode, 0));
                break;
            case 'e':
                mode_l.push_back(std::tuple<opmode_t, unsigned int>
                        (oqt_mode, 0));
                break;
            default:
                exit_fprintf_usage(argv);
                break;
        }
    }

    if (global_prefix_arg == NULL)
    {
        exit_fprintf_usage(argv);
    }
    else
    {
        prefix = std::string(global_prefix_arg);
        //FIXME: Get rid of this when moving csv/qsi I/O stuff to iostreams
        input_fp = fopen((prefix + ".csv").c_str(), "r");
    }

    if (build_mode == 1)
    {
        n_qtree* tree;
        int junk_data = 1;

        tree = read_qtree(input_fp, &junk_data);
        link_nodes_morton(tree);
        if (print_mode == 1)
        {
            print_qtree_integerwise(tree, 0);
        }

        while (!mode_l.empty())
        {
            switch (std::get<0>(mode_l.front()))
            {
                case qsi_mode:
                    qsiseq = qsiseq_from_n_qtree(tree, std::get<1>
                            (mode_l.front()));
                    tree_file = std::fstream((prefix + ".qsi_" +
                                std::to_string(std::get<1>(mode_l.front()))
                                ).c_str(),
                            std::fstream::binary | std::fstream::out |
                            std::fstream::trunc);
                    write_qsiseq(qsiseq, tree_file);
                    tree_file.flush();
                    tree_file.close();
                    if (print_mode == 1)
                    {
                        printf("qsi:\n");
                        pprint_qsiseq(qsiseq);
                    }
                    free_qsiseq(qsiseq);
                    break;
                case bqt_mode:
                    bitqtree = BitQTree(tree);
                    tree_file = std::fstream((prefix + ".bqt").c_str(),
                            std::fstream::binary | std::fstream::out |
                            std::fstream::trunc);
                    bitqtree.serialize(tree_file);
                    tree_file.flush();
                    tree_file.close();
                    //TODO: bqt print mode?
                    break;
                case oqt_mode:
                    //FIXME: It'd be faster have a constructor that takes
                    //       a n_qtree*.
                    bitqtree = BitQTree(tree);
                    oqt = OffsetQTree<unsigned int>(&bitqtree);
                    tree_file = std::fstream((prefix + ".oqt").c_str(),
                            std::fstream::binary | std::fstream::out |
                            std::fstream::trunc);
                    oqt.serialize(tree_file);
                    tree_file.flush();
                    tree_file.close();
                    if (print_mode == 1)
                    {
                        printf("oqt:\n");
                        oqt.pprint();
                    }
                    break;
            }
            mode_l.pop_front();
        }

        free_qtree(tree, 1);
        exit(EXIT_SUCCESS);
    }

    if (build_mode == 0)
    {
        long int slow_ly, fast_ly, slow_time, fast_time;
        long int time_diff;
        struct timeval flag1, flag2, flag3;
        unsigned int lox, loy, hix, hiy;

        // null_mode used here to indicate some sort of error.
        std::tuple<opmode_t, unsigned int> mode_flag =
            std::tuple<opmode_t, unsigned int>(null_mode, 0);

        // Work out which mode we should be in -- Multiple can be specified,
        // but only the final one matters.
        if (mode_l.empty())
        {
            exit_printf_usage(argv);
        }
        mode_flag = mode_l.back();
        if (std::get<0>(mode_flag) == null_mode)
        {
            exit_printf_usage(argv);
        }

        switch (std::get<0>(mode_flag))
        {
            case qsi_mode:
                tree_file = std::fstream((prefix + ".qsi_" +
                            std::to_string(std::get<1>(mode_l.front()))
                            ).c_str(),
                        std::fstream::binary | std::fstream::in);
                qsiseq = read_qsiseq(tree_file);
                break;
            case bqt_mode:
                tree_file = std::fstream((prefix + ".bqt").c_str(),
                        std::fstream::binary | std::fstream::in);
                bqtree.load(tree_file);
                break;
            case oqt_mode:
                tree_file = std::fstream((prefix + ".oqt").c_str(),
                        std::fstream::binary | std::fstream::in);
                oqtree.load(tree_file);
                break;
        }

        if (print_mode == 1)
        {
            switch (std::get<0>(mode_flag))
            {
                case qsi_mode:
                    pprint_qsiseq(qsiseq);
                    break;
                case bqt_mode:
                    printf("bqt pprint() not implemented.\n");
                    break;
                case oqt_mode:
                    oqtree.pprint();
                    break;
            }
        }

        lox = loy = hix = hiy = 0;
        while (read_query_range(input_fp, &lox, &loy, &hix, &hiy) != EOF)
        {
            printf("%u,%u %u,%u: ", lox, loy, hix, hiy);
            switch (std::get<0>(mode_flag))
            {
                case qsi_mode:
                    printf("%lu\n", fast_lee_yang_qsi(qsiseq,
                                lox, loy, hix, hiy));
                    break;
                case bqt_mode:
                    printf("bqt querying not implemented.\n");
                    break;
                case oqt_mode:
                    //FIXME: This
                    printf("oqt querying not implemented.\n");
                    break;
            }
        }

        /* FIXME: Re-implement timing stuff:
         *
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
        */
    }

    exit(EXIT_SUCCESS);
}
