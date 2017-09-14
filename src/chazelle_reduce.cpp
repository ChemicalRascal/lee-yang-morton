/* vim: set et sts=4 sw=4 cc=80 tw=80: */

/*******************************************
 *
 * Takes a list of coordinates (doubles),
 * and outputs another list (luints), which
 * has been reduced.
 *
 *  .csv_raw -> .csv
 *  Produces:
 *      FIXME: Actually output
 *              $PREFIX.chazelle_{x,y}
 *      .chazelle_x
 *          - std::vec<double>
 *      .chazelle_y
 *          - std::vec<double>
 *
 *******************************************/

#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <stdarg.h>
#include <tuple>
#include <vector>
#include <algorithm>
#include <fstream>

#include "read_csv.h"

#include <unistd.h>
#include <string.h>

#define DEFAULT_VEC_SIZE    0

// Apparently the cool kids don't use defines anymore
//
// I'm mainly using this many define-things because it's late and thinking is
// hard
using vec_size_type = uint64_t;
using fp_type = double;
using fp_map_type = std::vector<fp_type>;
using fp_tup_type = std::tuple<fp_type, fp_type>;
using fp_vec_type = std::vector<fp_tup_type>;
using r_tup_type = std::tuple<vec_size_type, vec_size_type>;
using r_vec_type = std::vector<r_tup_type>;

int         global_quiet_mode;
char*       global_prefix_arg;
fp_vec_type global_v;

/* Returns EOF if things went badly.
 */
int
read_double_coord(FILE* fp, double* x, double* y)
{
    double double_read, double_read_2;
    if (readcsv_get_double(fp, &double_read) != EOF)
    {
        if (readcsv_get_double(fp, &double_read_2) != EOF)
        {
            *x = double_read;
            *y = double_read_2;
            return 0;
        }
    }
    return EOF;
}

std::vector<std::tuple<fp_type, fp_type>>
read_csv_raw_to_fp_vector(FILE* fp)
{
    fp_type x, y;
    fp_vec_type v = fp_vec_type(DEFAULT_VEC_SIZE);

    while (read_double_coord(fp, &x, &y) != EOF)
    {
        v.push_back(std::tuple<fp_type, fp_type>(x, y));
    }
    v.shrink_to_fit();
    return v;
}

void
print_fp_vector(fp_vec_type& v)
{
    fp_vec_type::iterator i;
    for (i = v.begin(); i != v.end(); i = std::next(i))
    {
        printf("%lf, %lf\n", std::get<0>(*i), std::get<1>(*i));
    }
}

void
exit_fprintf_help(char** argv)
{
    fprintf(stdout, "Usage: %s -x [PREFIX]\n", argv[0]);
    fprintf(stdout, "Performs Chazelle reduction.\n");
    fprintf(stdout, "\n");
    fprintf(stdout, "  -x PREFIX must be provided.\n");
    exit(EXIT_SUCCESS);
}

void
exit_fprintf_usage(char** argv)
{
    fprintf(stderr, "Usage: %s -x [PREFIX]\n", argv[0]);
    fprintf(stderr, "Try '%s --help' for more information.\n", argv[0]);
    exit(EXIT_FAILURE);
}

template<vec_size_type t_i>
void
chazelle_sort(std::vector<vec_size_type>& v)
{
    std::sort(v.begin(), v.end(),
            [](const vec_size_type a, const vec_size_type b)
            {
                return (std::get<t_i>(global_v[a-1])
                        < std::get<t_i>(global_v[b-1]));
            });
}

r_tup_type
chazelle_translate(fp_map_type& c_x, fp_map_type& c_y, fp_tup_type& a,
        bool high = false)
{
    vec_size_type rx, ry;
    fp_type ax, ay;
    ax = std::get<0>(a);
    ay = std::get<1>(a);

    if (ax < c_x.front()) { rx = 0; }
    else if (ax > c_x.back()) { rx = c_x.size() + 1; }
    else
    {
        rx = (std::lower_bound(c_x.cbegin(), c_x.cend(), ax) - c_x.cbegin())+1;
        // Handle ineq. for hix,hiy range search coord (lox,loy doesn't need
        // adjustment).
        if (high && ax != c_x[rx-1]) { rx -= 1; }
    }
    if (ay < c_y.front()) { ry = 0; }
    else if (ay > c_y.back()) { ry = c_y.size() + 1; }
    else
    {
        ry = (std::lower_bound(c_y.cbegin(), c_y.cend(), ay) - c_y.cbegin())+1;
        if (high && ay != c_y[ry-1]) { ry -= 1; }
    }

    return r_tup_type{rx, ry};
}

void
chazelle_reduce(fp_map_type& c_x, fp_map_type& c_y, r_vec_type& reduced)
{
    // Note the distinction between cx and c_x
    std::vector<vec_size_type> cx(0), cy(0);
    vec_size_type i;

    for (i = 1; i <= global_v.size(); i++)
    { cx.push_back(i); cy.push_back(i); }
    chazelle_sort<0>(cx);
    chazelle_sort<1>(cy);

    for (i = 0; i < global_v.size(); i++)
    {
        c_x.push_back(std::get<0>(global_v[cx[i]-1]));
        c_y.push_back(std::get<1>(global_v[cy[i]-1]));
    }

    for (i = 0; i < global_v.size(); i++)
    {
        reduced[i] = chazelle_translate(c_x, c_y, global_v[i]);
    }
}

int
main(int argc, char** argv, char** envp)
{
    int opt;
    FILE* input_fp;
    std::string prefix;
    std::vector<fp_type> c_x, c_y;
    r_vec_type reduced;
    std::ofstream out_file;

    vec_size_type i;

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
    }

    global_quiet_mode = 0;
    global_prefix_arg = NULL;
    input_fp = NULL;

    while ((opt = getopt(argc, argv, "x:")) != -1)
    {
        switch (opt)
        {
            case 'x':
                global_prefix_arg = optarg;
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
        input_fp = fopen((prefix + ".csv_raw").c_str(), "r");
    }

    global_v = read_csv_raw_to_fp_vector(input_fp);
    c_x = std::vector<fp_type>(DEFAULT_VEC_SIZE);
    c_y = std::vector<fp_type>(DEFAULT_VEC_SIZE);
    reduced = r_vec_type(global_v.size());
    chazelle_reduce(c_x, c_y, reduced);

    out_file.open((prefix + ".csv").c_str(),
            std::fstream::out | std::fstream::trunc);

    for (i = 0; i < global_v.size(); i++)
    {
        out_file << std::to_string(std::get<0>(reduced[i])) << ", " <<
                    std::to_string(std::get<1>(reduced[i])) << std::endl;
    }
    out_file.flush();
    out_file.close();

    //We know what these are
    out_file.open((prefix + ".min_x").c_str(),
            std::fstream::binary | std::fstream::out |
            std::fstream::trunc);
    out_file << std::to_string(1) << std::endl;
    out_file.flush();
    out_file.close();
    out_file.open((prefix + ".max_x").c_str(),
            std::fstream::binary | std::fstream::out |
            std::fstream::trunc);
    out_file << std::to_string(global_v.size()) << std::endl;
    out_file.flush();
    out_file.close();
    out_file.open((prefix + ".min_y").c_str(),
            std::fstream::binary | std::fstream::out |
            std::fstream::trunc);
    out_file << std::to_string(1) << std::endl;
    out_file.flush();
    out_file.close();
    out_file.open((prefix + ".max_y").c_str(),
            std::fstream::binary | std::fstream::out |
            std::fstream::trunc);
    out_file << std::to_string(global_v.size()) << std::endl;
    out_file.flush();
    out_file.close();

    exit(EXIT_SUCCESS);
}
