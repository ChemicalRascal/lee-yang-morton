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
#include "k2_range.hpp"

#include <limits.h>

#include <unistd.h>
#include <string.h>
#include <sys/time.h>

#include <sdsl/vectors.hpp>

#define fprintf_if_eq(a, b, fp, args...) {if((a)==(b)){fprintf((fp), args);}}
#define q_fprintf(fp, args...) fprintf_if_eq(global_quiet_mode, 0, fp, args)
#define q_fprintf_if_eq(a, b, fp, args...) {if((a)==(b))\
    {q_fprintf((fp), args);}}

#define DEFAULT_VEC_SIZE 0
#define vec_size_type uint64_t

int     global_quiet_mode;
char*   global_prefix_arg;

enum opmode_t
{
    null_mode,      //
    qsi_mode,       // -c
    bqt_mode,       // -d
    oqt_mode,       // -e
    sdsl_k2_mode,   // -f
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

std::vector<std::tuple<vec_size_type, vec_size_type>>
read_csv_to_vector(FILE* fp, unsigned int* size)
{
    unsigned int x, y;
    std::vector<std::tuple<vec_size_type, vec_size_type>> v(DEFAULT_VEC_SIZE);

    //Skip depth
    readcsv_get_uint(fp, &x);
    if (size != NULL)
    {
        *size = 0;
    }
    while (read_coord(fp, &x, &y) != EOF)
    {
        v.push_back(std::tuple<vec_size_type, vec_size_type>(x, y));
        if (size != NULL && *size < std::max(x, y))
        {
            *size = std::max(x, y);
        }
    }
    v.shrink_to_fit();
    // k2 expects values within [0, size)
    *size += 1;
    return v;
}

//std::vector<std::tuple<vec_size_type, vec_size_type>>
void
sort_coord_vector(std::vector<std::tuple<vec_size_type, vec_size_type>>& v)
{
    std::sort(v.begin(), v.end(), [](std::tuple<vec_size_type, vec_size_type>a,
                std::tuple<vec_size_type, vec_size_type> b)
            {
                uint64_t za, zb;
                morton_PtoZ(std::get<0>(a), std::get<1>(a), &za);
                morton_PtoZ(std::get<0>(b), std::get<1>(b), &zb);
                return za < zb;
            });
}

void
print_coord_vector(std::vector<std::tuple<vec_size_type, vec_size_type>> v)
{
    std::vector<std::tuple<vec_size_type, vec_size_type>>::iterator i;
    for (i = v.begin(); i != v.end(); i = std::next(i))
    {
        printf("%lu, %lu\n", std::get<0>(*i), std::get<1>(*i));
    }
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
    fprintf(stdout, "Usage: %s [OPTIONS]... -x [PREFIX]\n", argv[0]);
    fprintf(stdout, "Perform range queries using the Lee-Yang algorithm.\n");
    fprintf(stdout, "\n");
    fprintf(stdout, "  -x PREFIX must be provided.\n");
    fprintf(stdout, "\n");
    fprintf(stdout, "  -q           quiet mode\n");
    fprintf(stdout, "  -p           print mode\n");
    fprintf(stdout, "  -b           build mode (otherwise, search mode)\n");
    fprintf(stdout, "  -t           timing mode\n");
    fprintf(stdout, "\n");
    fprintf(stdout, "  -c Q         QSI w/ psum quantum of Q:   .qsi_$Q\n");
    fprintf(stdout, "  -d           Binary Quad Tree:           .bqt\n");
    fprintf(stdout, "  -e           Offset Quad Tree:           .oqt\n");
    fprintf(stdout, "  -f           SDSL k2 Tree:               .k2\n");
    exit(EXIT_SUCCESS);
}

void
exit_fprintf_usage(char** argv)
{
    //FIXME: Update this
    fprintf(stderr, "Usage: %s [OPTIONS]... -x [PREFIX]\n", argv[0]);
    fprintf(stderr, "Try '%s --help' for more information.\n", argv[0]);
    exit(EXIT_FAILURE);
}

int
main(int argc, char** argv, char** envp)
{
    int opt, build_mode, print_mode, timing_mode;
    FILE* input_fp;
    unsigned int maxatt;

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
    
    const unsigned int k = 2;
    k2_range<k> k2;

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

    while ((opt = getopt(argc, argv, "bqpx:tc:def")) != -1)
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
            case 'f':
                mode_l.push_back(std::tuple<opmode_t, unsigned int>
                        (sdsl_k2_mode, 0));
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

        std::vector<std::tuple<vec_size_type, vec_size_type>> coord_vec =
            read_csv_to_vector(input_fp, &maxatt);
        sort_coord_vector(coord_vec);
        rewind(input_fp);

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
                case null_mode:
                    exit_fprintf_usage(argv);
                    break;
                case qsi_mode:
                    qsiseq = qsiseq_from_c_vec(coord_vec, std::get<1>
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
                case sdsl_k2_mode:
                    // TODO: Work out how to handle multiple different ks at
                    // once
                    //k = std::get<1>(mode_l.front());
                    k2 = k2_range<k>(coord_vec, maxatt);
                    tree_file = std::fstream((prefix + ".k2").c_str(),
                            std::fstream::binary | std::fstream::out |
                            std::fstream::trunc);
                    k2.serialize(tree_file);
                    tree_file.flush();
                    tree_file.close();
                    tree_file = std::fstream((prefix + ".k2_size").c_str(),
                            std::fstream::binary | std::fstream::out |
                            std::fstream::trunc);
                    tree_file << std::to_string(maxatt) << std::endl;
                    tree_file.flush();
                    tree_file.close();
                    break;
                default:
                    break;
            }
            mode_l.pop_front();
        }

        free_qtree(tree, 1);
        exit(EXIT_SUCCESS);
    }

    if (build_mode == 0)
    {
        //FIXME: Unused vars for timing stuff
        //long int slow_ly, fast_ly, slow_time, fast_time;
        //long int time_diff;
        //struct timeval flag1, flag2, flag3;
        unsigned int lox, loy, hix, hiy;

        // null_mode used here to indicate some sort of error.
        std::tuple<opmode_t, unsigned int> mode_flag =
            std::tuple<opmode_t, unsigned int>(null_mode, 0);

        // Work out which mode we should be in -- Multiple can be specified,
        // but only the final one matters.
        if (mode_l.empty())
        {
            exit_fprintf_usage(argv);
        }
        mode_flag = mode_l.back();
        if (std::get<0>(mode_flag) == null_mode)
        {
            exit_fprintf_usage(argv);
        }

        if (timing_mode == 1)
        {
            printf("Timing mode isn't currently implemented, go away\n");
            exit_fprintf_usage(argv);
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
                bitqtree.load(tree_file);
                break;
            case oqt_mode:
                tree_file = std::fstream((prefix + ".oqt").c_str(),
                        std::fstream::binary | std::fstream::in);
                oqt.load(tree_file);
                break;
            case sdsl_k2_mode:
                tree_file = std::fstream((prefix + ".k2").c_str(),
                        std::fstream::binary | std::fstream::in);
                k2.load(tree_file);
                readcsv_get_uint(fopen((prefix + ".k2_size").c_str(), "r"),
                        &maxatt);
                break;
            default:
                exit_fprintf_usage(argv);
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
                    oqt.pprint();
                    break;
                case sdsl_k2_mode:
                    printf("k2 pprint() not implemented.\n");
                    break;
                default:
                    exit_fprintf_usage(argv);
                    break;
            }
        }

        lox = loy = hix = hiy = 0;
        //while (read_query_range(input_fp, &lox, &loy, &hix, &hiy) != EOF)
        while (read_query_range(stdin, &lox, &loy, &hix, &hiy) != EOF)
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
                case sdsl_k2_mode:
                    printf("%lu\n", k2.range_count(lox, hix, loy, hiy));
                    break;
                default:
                    exit_fprintf_usage(argv);
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
