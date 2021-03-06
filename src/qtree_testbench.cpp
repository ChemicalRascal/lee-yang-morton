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
#include <errno.h>

#include "bitseq.hpp"
#include "qsi.hpp"
#include "leeyang.hpp"
#include "read_csv.h"
#include "morton.hpp"
#include "bit_qtree.hpp"

#include "offset_qtree.hpp"
#include "k2_range.hpp"
#include "offset_finkel_bentley.hpp"

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

//FIXME: Move n_qtree to a coord-vector-built-system (with ofb n_qtree isn't
//       really nessecary though, consider removing entirely)
#define HARDCODED_N_QTREE_DEPTH 6

int     global_quiet_mode;
char*   global_prefix_arg;

enum opmode_t
{
    null_mode,      //
    qsi_mode,       // -c
    bqt_mode,       // -d
    oqt_mode,       // -e
    sdsl_k2_mode,   // -f
    ofb_mode,       // -g
    iter_k2_mode,   // -h
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
read_csv_to_vector(FILE* fp, std::tuple<unsigned, unsigned, unsigned,
        unsigned>& bounds)
{
    unsigned int x, y;
    std::vector<std::tuple<vec_size_type, vec_size_type>> v(DEFAULT_VEC_SIZE);

    //Skip depth
    //readcsv_get_uint(fp, &x);
    //min x, max x, min y, max y
    std::get<0>(bounds) = (unsigned)-1;
    std::get<1>(bounds) = 0;
    std::get<2>(bounds) = (unsigned)-1;
    std::get<3>(bounds) = 0;
    while (read_coord(fp, &x, &y) != EOF)
    {
        v.push_back(std::tuple<vec_size_type, vec_size_type>(x, y));
        if (x < std::get<0>(bounds)) std::get<0>(bounds) = x;
        if (x > std::get<1>(bounds)) std::get<1>(bounds) = x;
        if (y < std::get<2>(bounds)) std::get<2>(bounds) = y;
        if (y > std::get<3>(bounds)) std::get<3>(bounds) = y;
    }
    v.shrink_to_fit();
    // k2 expects values within [0, size)
    std::get<1>(bounds) += 1;
    std::get<3>(bounds) += 1;
    return v;
}

std::vector<std::tuple<vec_size_type, vec_size_type, vec_size_type,
    vec_size_type, vec_size_type>>
read_range_queries_to_vector(FILE* fp)
{
    unsigned int lox, loy, hix, hiy;
    std::vector<std::tuple<vec_size_type, vec_size_type, vec_size_type,
        vec_size_type, vec_size_type>> v(DEFAULT_VEC_SIZE);

    while (read_query_range(fp, &lox, &loy, &hix, &hiy) != EOF)
    {
        v.push_back(std::tuple<vec_size_type, vec_size_type, vec_size_type,
                vec_size_type, vec_size_type>(lox, loy, hix, hiy, 0));
    }
    v.shrink_to_fit();
    return v;
}

void
clamp_query_vector(std::vector<std::tuple<vec_size_type, vec_size_type,
        vec_size_type, vec_size_type, vec_size_type>>& qv,
        std::tuple<unsigned, unsigned, unsigned, unsigned> bounds)
{
    auto q_i = qv.begin();
    for (; q_i != qv.end(); q_i++)
    {
        //lox
        if (std::get<0>(*q_i) < std::get<0>(bounds))
        { std::get<0>(*q_i) = std::get<1>(bounds); }
        //loy
        if (std::get<1>(*q_i) < std::get<2>(bounds))
        { std::get<1>(*q_i) = std::get<2>(bounds); }
        //hix
        if (std::get<2>(*q_i) > std::get<1>(bounds) - 1)
        { std::get<2>(*q_i) = std::get<1>(bounds) - 1; }
        //hiy
        if (std::get<3>(*q_i) > std::get<3>(bounds) - 1)
        { std::get<3>(*q_i) = std::get<3>(bounds) - 1; }
    }
}

vec_size_type
range_search_linear_scan(
        const std::vector<std::tuple<vec_size_type, vec_size_type>>& coords,
        const std::tuple<vec_size_type, vec_size_type, vec_size_type,
        vec_size_type, vec_size_type>& q
        )
{
    vec_size_type count = 0;
    auto c_i = coords.cbegin();
    for (; c_i != coords.cend(); c_i++)
    {
        if (
                std::get<0>(*c_i) >= std::get<0>(q) &&
                std::get<0>(*c_i) <= std::get<2>(q) &&
                std::get<1>(*c_i) >= std::get<1>(q) &&
                std::get<1>(*c_i) <= std::get<3>(q)
           )
        {
            count += 1;
        }
    }
    return count;
}

//std::vector<std::tuple<vec_size_type, vec_size_type>>
void
sort_coord_vector(std::vector<std::tuple<vec_size_type, vec_size_type>>& v)
{
    std::sort(v.begin(), v.end(), [](std::tuple<vec_size_type, vec_size_type>a,
                std::tuple<vec_size_type, vec_size_type> b)
            {
                uint64_t za, zb;
                morton::PtoZ(std::get<0>(a), std::get<1>(a), za);
                morton::PtoZ(std::get<0>(b), std::get<1>(b), zb);
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
//TODO: Move this to using coordinate vectors -- if can be bothered?
n_qtree*
read_qtree(FILE* fp, void* data)
{
    n_qtree* tree;
    unsigned int x, y;

    tree = new_qtree(HARDCODED_N_QTREE_DEPTH);

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
            morton::PtoZ(i, j, z);
            printf("%2lu ", z);
        }
        printf("\n");
    }

    for (z = 0; z > 4096; z++)
    {
        morton::ZtoP(z, xa, ya);
        morton::ZtoP(z, xb, yb);
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
    fprintf(stdout, "  -v           validation mode\n");
    fprintf(stdout, "\n");
    fprintf(stdout, "  -c Q         QSI w/ psum quantum of Q:   .qsi_$Q\n");
    fprintf(stdout, "  -d           Binary Quad Tree:           .bqt\n");
    fprintf(stdout, "  -e           Offset Quad Tree:           .oqt\n");
    fprintf(stdout, "  -f K, 1<K<7  SDSL k2 Tree w/ k=K:        .k2_$K\n");
    fprintf(stdout, "  -g           Offset Finkel-Bentley       .ofb\n");
    fprintf(stdout, "  -h K, 1<K<7  SDSL k2 Tree w/ k=K:        .k2_$K\n");
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
    int opt, build_mode, print_mode, timing_mode, validation_mode;
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
    OffsetFBTree<> ofb;

    /* because k2 tree structure is a template class, this needs to exist,
     * sadly
     */
    unsigned int k;
    k2_range<2> k2_2;
    k2_range<3> k2_3;
    k2_range<4> k2_4;
    k2_range<5> k2_5;
    k2_range<6> k2_6;

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
    validation_mode = 0;
    global_prefix_arg = NULL;
    input_fp = NULL;

    while ((opt = getopt(argc, argv, "bqpx:tvc:def:gh:")) != -1)
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
            case 'v':
                validation_mode = 1;
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
                        (sdsl_k2_mode, atoi(optarg)));
                break;
            case 'g':
                mode_l.push_back(std::tuple<opmode_t, unsigned int>
                        (ofb_mode, 0));
                break;
            case 'h':
                mode_l.push_back(std::tuple<opmode_t, unsigned int>
                        (iter_k2_mode, atoi(optarg)));
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
        if (input_fp == NULL)
        {
            printf("Error: %s\n", strerror(errno));
            exit_fprintf_usage(argv);
        }
    }

    if (build_mode == 1)
    {
        //n_qtree* tree;
        //int junk_data = 1;

        std::tuple<unsigned, unsigned, unsigned, unsigned> bounds;

        std::vector<std::tuple<vec_size_type, vec_size_type>> coord_vec =
            read_csv_to_vector(input_fp, bounds);
        sort_coord_vector(coord_vec);
        rewind(input_fp);

        maxatt = std::max(std::get<1>(bounds), std::get<3>(bounds));
        tree_file = std::fstream((prefix + ".maxatt").c_str(),
                std::fstream::binary | std::fstream::out |
                std::fstream::trunc);
        tree_file << std::to_string(maxatt) << std::endl;
        tree_file.flush();
        tree_file.close();

        tree_file = std::fstream((prefix + ".min_x").c_str(),
                std::fstream::binary | std::fstream::out |
                std::fstream::trunc);
        tree_file << std::to_string(std::get<0>(bounds)) << std::endl;
        tree_file.flush();
        tree_file.close();
        tree_file = std::fstream((prefix + ".max_x").c_str(),
                std::fstream::binary | std::fstream::out |
                std::fstream::trunc);
        tree_file << std::to_string(std::get<1>(bounds)) << std::endl;
        tree_file.flush();
        tree_file.close();
        tree_file = std::fstream((prefix + ".min_y").c_str(),
                std::fstream::binary | std::fstream::out |
                std::fstream::trunc);
        tree_file << std::to_string(std::get<2>(bounds)) << std::endl;
        tree_file.flush();
        tree_file.close();
        tree_file = std::fstream((prefix + ".max_y").c_str(),
                std::fstream::binary | std::fstream::out |
                std::fstream::trunc);
        tree_file << std::to_string(std::get<3>(bounds)) << std::endl;
        tree_file.flush();
        tree_file.close();

        /*
        tree = read_qtree(input_fp, &junk_data);
        link_nodes_morton(tree);
        if (print_mode == 1)
        {
            //print_qtree_integerwise(tree, 0);
        }
        */

        while (!mode_l.empty())
        {
            switch (std::get<0>(mode_l.front()))
            {
                case null_mode:
                    exit_fprintf_usage(argv);
                    break;
                case qsi_mode:
                    sort_coord_vector(coord_vec);
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
                    /* FIXME
                    bitqtree = BitQTree(tree);
                    tree_file = std::fstream((prefix + ".bqt").c_str(),
                            std::fstream::binary | std::fstream::out |
                            std::fstream::trunc);
                    bitqtree.serialize(tree_file);
                    tree_file.flush();
                    tree_file.close();
                    */
                    //TODO: bqt print mode?
                    break;
                case oqt_mode:
                    //FIXME: It'd be faster have a constructor that takes
                    //       a n_qtree*.
                    /* FIXME: It'd be faster have a constructor that takes
                     *       coord_vec.
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
                    */
                    break;
                case sdsl_k2_mode:
                    k = std::get<1>(mode_l.front());
                    tree_file = std::fstream((prefix + ".k2_"
                                + std::to_string(k)).c_str(),
                            std::fstream::binary | std::fstream::out |
                            std::fstream::trunc);
                    switch (k)
                    {
                        case 2:
                            k2_2 = k2_range<2>(coord_vec, maxatt);
                            k2_2.serialize(tree_file);
                            break;
                        case 3:
                            k2_3 = k2_range<3>(coord_vec, maxatt);
                            k2_3.serialize(tree_file);
                            break;
                        case 4:
                            k2_4 = k2_range<4>(coord_vec, maxatt);
                            k2_4.serialize(tree_file);
                            break;
                        case 5:
                            k2_5 = k2_range<5>(coord_vec, maxatt);
                            k2_5.serialize(tree_file);
                            break;
                        case 6:
                            k2_6 = k2_range<6>(coord_vec, maxatt);
                            k2_6.serialize(tree_file);
                            break;
                        default:
                            fprintf(stderr, "k: %u not supported\n", k);
                            break;
                    }
                    tree_file.flush();
                    tree_file.close();
                    break;
                case iter_k2_mode:
                    fprintf(stderr, "Building iter_k2 is not supported\n");
                    break;
                case ofb_mode:
                    ofb = OffsetFBTree<>(coord_vec);
                    tree_file = std::fstream((prefix + ".ofb").c_str(),
                            std::fstream::binary | std::fstream::out |
                            std::fstream::trunc);
                    ofb.serialize(tree_file);
                    if (print_mode == 1)
                    {
                        printf("ofb:\n");
                        ofb.pprint();
                    }
                    tree_file.flush();
                    tree_file.close();
                    break;
                default:
                    fprintf(stderr, "Mode not supported");
                    break;
            }
            mode_l.pop_front();
        }

        //free_qtree(tree, 1);
        exit(EXIT_SUCCESS);
    }

    if (build_mode == 0)
    {
        long int batch_sec, batch_usec;
        struct timeval t_00, t_01, tsub, ts_00, ts_01;

        // null_mode used here to indicate some sort of error.
        std::tuple<opmode_t, unsigned int> mode_flag =
            std::tuple<opmode_t, unsigned int>(null_mode, 0);

        if (mode_l.empty())
        {
            exit_fprintf_usage(argv);
        }

        // Bounds tuple
        // TODO: Move bounds from unit to luint
        std::tuple<unsigned, unsigned, unsigned, unsigned> bounds;
        readcsv_get_uint(fopen((prefix + ".min_x").c_str(), "r"), &maxatt);
        std::get<0>(bounds) = maxatt;
        readcsv_get_uint(fopen((prefix + ".max_x").c_str(), "r"), &maxatt);
        std::get<1>(bounds) = maxatt;
        readcsv_get_uint(fopen((prefix + ".min_y").c_str(), "r"), &maxatt);
        std::get<2>(bounds) = maxatt;
        readcsv_get_uint(fopen((prefix + ".max_y").c_str(), "r"), &maxatt);
        std::get<3>(bounds) = maxatt;

        readcsv_get_uint(fopen((prefix + ".maxatt").c_str(), "r"), &maxatt);

        auto mode_i = mode_l.cbegin();
        for (; mode_i != mode_l.cend(); mode_i++)
        {
            switch (std::get<0>(*mode_i))
            {
                case qsi_mode:
                    tree_file = std::fstream((prefix + ".qsi_" +
                                std::to_string(std::get<1>(*mode_i))
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
                case iter_k2_mode:
                    k = std::get<1>(*mode_i);
                    tree_file = std::fstream((prefix + ".k2_"
                                + std::to_string(k)).c_str(),
                            std::fstream::binary | std::fstream::in);
                    switch (k)
                    {
                        case 2:
                            k2_2.load(tree_file);
                            break;
                        case 3:
                            k2_3.load(tree_file);
                            break;
                        case 4:
                            k2_4.load(tree_file);
                            break;
                        case 5:
                            k2_5.load(tree_file);
                            break;
                        case 6:
                            k2_6.load(tree_file);
                            break;
                        default:
                            fprintf(stderr, "k: %u not supported\n", k);
                            break;
                    }
                    break;
                case ofb_mode:
                    tree_file = std::fstream((prefix + ".ofb").c_str(),
                            std::fstream::binary | std::fstream::in);
                    ofb.load(tree_file);
                    break;
                default:
                    exit_fprintf_usage(argv);
                    break;
            }
        }

        // Code duplication is fun
        if (validation_mode != 1)
        {
            vec_size_type sum, xorfold;
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
                    case iter_k2_mode:
                        printf("k2 pprint() not implemented.\n");
                        break;
                    case ofb_mode:
                        ofb.pprint();
                        break;
                    default:
                        exit_fprintf_usage(argv);
                        break;
                }
            }

            //Read min/max attrs, build bounds tuple
            std::tuple<unsigned, unsigned, unsigned, unsigned> bounds;
            std::vector<std::tuple<vec_size_type, vec_size_type, vec_size_type,
                vec_size_type, vec_size_type>> query_vec =
                    read_range_queries_to_vector(stdin);
            std::vector<std::tuple<vec_size_type, vec_size_type, vec_size_type,
                vec_size_type, vec_size_type>>::iterator qvi;

            std::vector<std::pair<long unsigned int, long unsigned int>> tempv;

            sum = 0;
            xorfold = 0;
            switch (std::get<0>(mode_flag))
            {
                case qsi_mode:
                    gettimeofday(&t_00, NULL);
                    for (qvi = query_vec.begin(); qvi != query_vec.end(); qvi++)
                    {
                        std::get<4>(*qvi) = fast_lee_yang_qsi(qsiseq,
                                std::get<0>(*qvi), std::get<1>(*qvi),
                                std::get<2>(*qvi), std::get<3>(*qvi));
                        sum += std::get<4>(*qvi);
                        xorfold ^= std::get<4>(*qvi);
                    }
                    gettimeofday(&t_01, NULL);
                    break;
                case bqt_mode:
                    gettimeofday(&t_00, NULL);
                    printf("bqt querying not implemented.\n");
                    gettimeofday(&t_01, NULL);
                    break;
                case oqt_mode:
                    //FIXME: This
                    gettimeofday(&t_00, NULL);
                    printf("oqt querying not implemented.\n");
                    gettimeofday(&t_01, NULL);
                    break;
                case sdsl_k2_mode:
                    k = std::get<1>(mode_flag);
                    switch (k)
                    {
                        case 2:
                            gettimeofday(&t_00, NULL);
                            for (qvi = query_vec.begin();
                                    qvi != query_vec.end(); qvi++)
                            {
                                std::get<4>(*qvi) = k2_2.range_count(
                                        std::get<0>(*qvi), std::get<2>(*qvi),
                                        std::get<1>(*qvi), std::get<3>(*qvi));
                                sum += std::get<4>(*qvi);
                                xorfold ^= std::get<4>(*qvi);
                            }
                            gettimeofday(&t_01, NULL);
                            break;
                        case 3:
                            gettimeofday(&t_00, NULL);
                            for (qvi = query_vec.begin();
                                    qvi != query_vec.end(); qvi++)
                            {
                                std::get<4>(*qvi) = k2_3.range_count(
                                        std::get<0>(*qvi), std::get<2>(*qvi),
                                        std::get<1>(*qvi), std::get<3>(*qvi));
                                sum += std::get<4>(*qvi);
                                xorfold ^= std::get<4>(*qvi);
                            }
                            gettimeofday(&t_01, NULL);
                            break;
                        case 4:
                            gettimeofday(&t_00, NULL);
                            for (qvi = query_vec.begin();
                                    qvi != query_vec.end(); qvi++)
                            {
                                std::get<4>(*qvi) = k2_4.range_count(
                                        std::get<0>(*qvi), std::get<2>(*qvi),
                                        std::get<1>(*qvi), std::get<3>(*qvi));
                                sum += std::get<4>(*qvi);
                                xorfold ^= std::get<4>(*qvi);
                            }
                            gettimeofday(&t_01, NULL);
                            break;
                        case 5:
                            gettimeofday(&t_00, NULL);
                            for (qvi = query_vec.begin();
                                    qvi != query_vec.end(); qvi++)
                            {
                                std::get<4>(*qvi) = k2_5.range_count(
                                        std::get<0>(*qvi), std::get<2>(*qvi),
                                        std::get<1>(*qvi), std::get<3>(*qvi));
                                sum += std::get<4>(*qvi);
                                xorfold ^= std::get<4>(*qvi);
                            }
                            gettimeofday(&t_01, NULL);
                            break;
                        case 6:
                            gettimeofday(&t_00, NULL);
                            for (qvi = query_vec.begin();
                                    qvi != query_vec.end(); qvi++)
                            {
                                std::get<4>(*qvi) = k2_6.range_count(
                                        std::get<0>(*qvi), std::get<2>(*qvi),
                                        std::get<1>(*qvi), std::get<3>(*qvi));
                                sum += std::get<4>(*qvi);
                                xorfold ^= std::get<4>(*qvi);
                            }
                            gettimeofday(&t_01, NULL);
                            break;
                        default:
                            fprintf(stderr, "k: %u not supported\n", k);
                            break;
                    }
                    break;
                case iter_k2_mode:
                    k = std::get<1>(mode_flag);
                    tsub.tv_sec = 0;
                    tsub.tv_usec = 0;
                    switch (k)
                    {
                        case 2:
                            gettimeofday(&t_00, NULL);
                            for (qvi = query_vec.begin();
                                    qvi != query_vec.end(); qvi++)
                            {
                                gettimeofday(&ts_00, NULL);
                                tempv = k2_2.range(
                                        std::get<0>(*qvi), std::get<2>(*qvi),
                                        std::get<1>(*qvi), std::get<3>(*qvi));
                                gettimeofday(&ts_01, NULL);
                                std::get<4>(*qvi) = tempv.size();
                                sum += std::get<4>(*qvi);
                                xorfold ^= std::get<4>(*qvi);

                                // Handrolled timing nonsense to avoid having to
                                // time the .size() call
                                batch_sec = ts_01.tv_sec - ts_00.tv_sec;
                                batch_usec = ts_01.tv_usec - ts_00.tv_usec;
                                if (batch_usec < 0)
                                { batch_usec += 1000000; batch_sec -= 1; }
                                tsub.tv_sec += batch_sec;
                                tsub.tv_usec += batch_usec;
                                if (tsub.tv_usec >= 1000000)
                                { tsub.tv_usec -= 1000000; tsub.tv_sec += 1; }
                            }
                            // This is just the easiest way to incorporate the
                            // special snowflake timing code into the existing
                            // system. Hacks upon hacks, it's hacks all the way
                            // down, ecetera.
                            t_01.tv_sec = t_00.tv_sec + tsub.tv_sec;
                            t_01.tv_usec = t_00.tv_usec + tsub.tv_usec;
                            break;
                        case 3:
                            gettimeofday(&t_00, NULL);
                            for (qvi = query_vec.begin();
                                    qvi != query_vec.end(); qvi++)
                            {
                                std::get<4>(*qvi) = k2_3.range(
                                        std::get<0>(*qvi), std::get<2>(*qvi),
                                        std::get<1>(*qvi), std::get<3>(*qvi))
                                    .size();
                                sum += std::get<4>(*qvi);
                                xorfold ^= std::get<4>(*qvi);
                            }
                            gettimeofday(&t_01, NULL);
                            break;
                        case 4:
                            gettimeofday(&t_00, NULL);
                            for (qvi = query_vec.begin();
                                    qvi != query_vec.end(); qvi++)
                            {
                                std::get<4>(*qvi) = k2_4.range(
                                        std::get<0>(*qvi), std::get<2>(*qvi),
                                        std::get<1>(*qvi), std::get<3>(*qvi))
                                    .size();
                                sum += std::get<4>(*qvi);
                                xorfold ^= std::get<4>(*qvi);
                            }
                            gettimeofday(&t_01, NULL);
                            break;
                        case 5:
                            gettimeofday(&t_00, NULL);
                            for (qvi = query_vec.begin();
                                    qvi != query_vec.end(); qvi++)
                            {
                                std::get<4>(*qvi) = k2_5.range(
                                        std::get<0>(*qvi), std::get<2>(*qvi),
                                        std::get<1>(*qvi), std::get<3>(*qvi))
                                    .size();
                                sum += std::get<4>(*qvi);
                                xorfold ^= std::get<4>(*qvi);
                            }
                            gettimeofday(&t_01, NULL);
                            break;
                        case 6:
                            gettimeofday(&t_00, NULL);
                            for (qvi = query_vec.begin();
                                    qvi != query_vec.end(); qvi++)
                            {
                                std::get<4>(*qvi) = k2_6.range(
                                        std::get<0>(*qvi), std::get<2>(*qvi),
                                        std::get<1>(*qvi), std::get<3>(*qvi))
                                    .size();
                                sum += std::get<4>(*qvi);
                                xorfold ^= std::get<4>(*qvi);
                            }
                            gettimeofday(&t_01, NULL);
                            break;
                        default:
                            fprintf(stderr, "k: %u not supported\n", k);
                            break;
                    }
                    break;
                case ofb_mode:
                    gettimeofday(&t_00, NULL);
                    for (qvi = query_vec.begin(); qvi != query_vec.end(); qvi++)
                    {
                        std::get<4>(*qvi) = ofb.range_count(
                                std::get<0>(*qvi), std::get<2>(*qvi),
                                std::get<1>(*qvi), std::get<3>(*qvi));
                        sum += std::get<4>(*qvi);
                        xorfold ^= std::get<4>(*qvi);
                    }
                    gettimeofday(&t_01, NULL);
                    break;
                default:
                    gettimeofday(&t_00, NULL);
                    exit_fprintf_usage(argv);
                    gettimeofday(&t_01, NULL);
                    break;
            }
            batch_sec = t_01.tv_sec - t_00.tv_sec;
            batch_usec = t_01.tv_usec - t_00.tv_usec;
            if (batch_usec < 0)
            {
                batch_usec += 1000000;
                batch_sec -= 1;
            }

            if (timing_mode == 1)
            {
                switch (std::get<0>(mode_flag))
                {
                    case qsi_mode:
                        printf("qsi_%04d ", std::get<1>(mode_flag));
                        break;
                    case sdsl_k2_mode:
                        k = std::get<1>(mode_flag);
                        printf("k2_%04d  ", k);
                        break;
                    case iter_k2_mode:
                        k = std::get<1>(mode_flag);
                        printf("i2_%04d  ", k);
                        break;
                    case ofb_mode:
                        printf("ofb      ");
                        break;
                    default:
                        printf("???????? ");
                        break;
                }
                printf("%04ld.%06ld sec.usec, %lu, %lu\n", batch_sec,
                        batch_usec, sum, xorfold);
            }
            else
            {
                for (qvi = query_vec.begin(); qvi != query_vec.end(); qvi++)
                {
                    printf("%lu,%lu %lu,%lu: %lu\n",
                            std::get<0>(*qvi), std::get<1>(*qvi),
                            std::get<2>(*qvi), std::get<3>(*qvi),
                            std::get<4>(*qvi));
                }
            }
        } // if (validation_mode != 1)
        else if (validation_mode == 1)
        {
            printf("In val_mode\n");
            // Results vector
            std::vector<std::vector<vec_size_type>> r =
                std::vector<std::vector<vec_size_type>>();
            // Datapoints vector
            printf("reading csv to vector\n");
            rewind(input_fp);
            std::vector<std::tuple<vec_size_type, vec_size_type>> coord_vec =
                read_csv_to_vector(input_fp, bounds);
            rewind(input_fp);
            // Query vector
            printf("reading range queries to vector\n");
            std::vector<std::tuple<vec_size_type, vec_size_type, vec_size_type,
                vec_size_type, vec_size_type>> query_vec =
                    read_range_queries_to_vector(stdin);
            clamp_query_vector(query_vec, bounds);

            //TODO: With bounds tuple, clamp queries
            clamp_query_vector(query_vec, bounds);

            printf("linear scan start...\n");
            auto q_i = query_vec.cbegin();
            for (; q_i != query_vec.cend(); q_i++)
            {
                r.push_back(std::vector<vec_size_type>());
                (*(r.end()-1)).push_back(std::get<0>(*q_i));
                (*(r.end()-1)).push_back(std::get<1>(*q_i));
                (*(r.end()-1)).push_back(std::get<2>(*q_i));
                (*(r.end()-1)).push_back(std::get<3>(*q_i));
                (*(r.end()-1)).push_back(
                        range_search_linear_scan(coord_vec, *q_i)
                        );
            }
            printf(" done!\n");

            // Validation mode can handle multiple modes
            auto mode_i = mode_l.cbegin();
            for (; mode_i != mode_l.cend(); mode_i++)
            {
                gettimeofday(&t_00, NULL);
                auto r_i = r.begin();
                for (; r_i != r.end(); r_i++)
                {
                    switch (std::get<0>(*mode_i))
                    {
                        case qsi_mode:
                            (*r_i).push_back(
                                    fast_lee_yang_qsi(qsiseq,
                                        (*r_i)[0], (*r_i)[1],
                                        (*r_i)[2], (*r_i)[3]));
                            break;
                        case ofb_mode:
                            (*r_i).push_back(
                                    ofb.range_count(
                                        (*r_i)[0], (*r_i)[2],
                                        (*r_i)[1], (*r_i)[3]));
                            break;
                        case sdsl_k2_mode:
                            k = std::get<1>(*mode_i);
                            switch (k)
                            {
                                case 2:
                                    (*r_i).push_back(k2_2.range_count(
                                                (*r_i)[0], (*r_i)[2],
                                                (*r_i)[1], (*r_i)[3]));
                                    break;
                                case 3:
                                    (*r_i).push_back(k2_3.range_count(
                                                (*r_i)[0], (*r_i)[2],
                                                (*r_i)[1], (*r_i)[3]));
                                    break;
                                case 4:
                                    (*r_i).push_back(k2_4.range_count(
                                                (*r_i)[0], (*r_i)[2],
                                                (*r_i)[1], (*r_i)[3]));
                                    break;
                                case 5:
                                    (*r_i).push_back(k2_5.range_count(
                                                (*r_i)[0], (*r_i)[2],
                                                (*r_i)[1], (*r_i)[3]));
                                    break;
                                case 6:
                                    (*r_i).push_back(k2_6.range_count(
                                                (*r_i)[0], (*r_i)[2],
                                                (*r_i)[1], (*r_i)[3]));
                                    break;
                                default:
                                    fprintf(stderr, "k: %u not supported\n", 
                                            k);
                                    break;
                            }
                            break;
                        default:
                            break;
                    }
                }
                gettimeofday(&t_01, NULL);
                batch_sec = t_01.tv_sec - t_00.tv_sec;
                batch_usec = t_01.tv_usec - t_00.tv_usec;
                if (batch_usec < 0)
                {
                    batch_usec += 1000000;
                    batch_sec -= 1;
                }
                printf("%04ld.%06ld sec.usec\n", batch_sec, batch_usec);
            }

            vec_size_type ls_res, ineq_flag, master_ineq_flag = 0;
            auto r_i = r.cbegin();
            for (; r_i != r.cend(); r_i++)
            {
                ineq_flag = 0;
                ls_res = (*r_i)[4];
                // First five elements are lox, hix, loy, hiy, ls_res
                auto rr_i = r_i->cbegin() + 5;
                for (; rr_i != r_i->cend(); rr_i++)
                {
                    if (*rr_i != ls_res) { ineq_flag=1; master_ineq_flag++; }
                }
                if (ineq_flag == 1)
                {
                    rr_i = r_i->cbegin();
                    printf("%lu: ", r_i->size());
                    for (; rr_i != r_i->cend(); rr_i++)
                    {
                        printf("%lu", *rr_i);
                        if (rr_i != r_i->cend() - 1) { printf(", "); }
                    }
                    printf("\n");
                }
            }

            if (master_ineq_flag != 0)
            {
                printf("Errors found. n: %lu\n", master_ineq_flag);
                exit(EXIT_FAILURE);
            }
            printf("Looks like everything was fine?\n");
        }
    }

    exit(EXIT_SUCCESS);
}
