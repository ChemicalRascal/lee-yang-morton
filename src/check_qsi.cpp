/* vim: set et sts=4 sw=4 cc=80 tw=80: */

/*******************************************
 *
 * Iterates through a qsi sequence and checks
 *       if it's correct or not.
 *
 *******************************************/

#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <stdarg.h>
#include <list>
#include <tuple>

#include "qsi.hpp"
#include "read_csv.h"
#include "morton.hpp"

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

int     global_quiet_mode;
char*   global_prefix_arg;

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
    int opt, print_mode;
    FILE* input_fp;
    unsigned int qsi_q;

    std::string prefix;
    //FIXME: Currently unused
    //std::fstream csv_file;
    std::fstream tree_file;

    // vars used in various modes
    qsiseq* qsiseq;

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
    print_mode = 0;
    qsi_q = 0;
    global_prefix_arg = NULL;
    input_fp = NULL;

    while ((opt = getopt(argc, argv, "bqpx:tc:defg")) != -1)
    {
        switch (opt)
        {
            case 'q':
                global_quiet_mode = 1;
                break;
            case 'p':
                print_mode = 1;
                break;
            case 'x':
                global_prefix_arg = optarg;
                break;
            case 'c':
                qsi_q = atoi(optarg);
                break;
            default:
                exit_fprintf_usage(argv);
                break;
        }
    }

    if (global_prefix_arg == NULL || qsi_q == 0)
    {
        exit_fprintf_usage(argv);
    }
    else
    {
        prefix = std::string(global_prefix_arg);
        //FIXME: Get rid of this when moving csv/qsi I/O stuff to iostreams
        input_fp = fopen((prefix + ".csv").c_str(), "r");
    }

    std::tuple<unsigned, unsigned, unsigned, unsigned> bounds;

    std::vector<std::tuple<vec_size_type, vec_size_type>> coord_vec =
        read_csv_to_vector(input_fp, bounds);
    sort_coord_vector(coord_vec);
    tree_file = std::fstream((prefix + ".qsi_" +
                std::to_string(qsi_q)).c_str(),
            std::fstream::binary | std::fstream::in);
    qsiseq = read_qsiseq(tree_file);

    std::vector<long unsigned int> m_vec{};
    long unsigned int m;
    for (auto v_i = coord_vec.begin(); v_i != coord_vec.end(); v_i++)
    {
        morton_PtoZ(std::get<0>(*v_i), std::get<1>(*v_i), &m);
        m_vec.push_back(m);
    }

    qsi_next_state qsistate;
    m = qsi_get(qsiseq, &qsistate, 0);
    auto m_i = m_vec.begin();
    for (m_i = m_vec.begin(); m_i != m_vec.end(); m_i++)
    {
        if (m == ULONG_MAX) { break; }
        printf("%lu -- %lu ", *m_i, m);
        if (*m_i != m) { printf("!!!"); }
        printf("\n");
        m = qsi_get_next(qsiseq, &qsistate);
    }
    for (; m_i != m_vec.end(); m_i++)
    {
        printf("%lu -- NULL !!!\n", *m_i);
    }
    while (m != ULONG_MAX)
    {
        printf("NULL -- %lu\n", m);
        m = qsi_get_next(qsiseq, &qsistate);
    }

    if (print_mode == 1)
    {
        pprint_qsiseq(qsiseq);
        auto v_i = coord_vec.begin();
        m_i = m_vec.begin();
        while (v_i != coord_vec.end() || m_i != m_vec.end())
        {
            printf("%lu,%lu", std::get<0>(*v_i), std::get<1>(*v_i));
            printf(": %lu\n", *m_i);
            v_i++; m_i++;
        }
    }

    exit(EXIT_SUCCESS);
}
