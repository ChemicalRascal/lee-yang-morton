/* vim: set et sts=4 sw=4 cc=80 tw=80: */

/*******************************************
 *
 * Query generator for qtree_testbench.cpp
 *
 *******************************************/

#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>

#include <random>

#include "read_csv.h"

void
exit_fprintf_help(char** argv)
{
    fprintf(stdout, "Usage: %s [OPTION]... -x [PREFIX] -n [NUM]\n", argv[0]);
    fprintf(stdout, "Generate a set of NUM queries.\n");
    fprintf(stdout, "\n");
    fprintf(stdout, "  -x PREFIX must be provided.\n");
    fprintf(stdout, "  -n NUM must be provided.\n");
    fprintf(stdout, "\n");
    fprintf(stdout, "  -f FILE      write to FILE instead of stdout\n");
    exit(EXIT_SUCCESS);
}

void
exit_fprintf_usage(char** argv)
{
    /*
     */
    fprintf(stdout, "Usage: %s [OPTION]... -x [PREFIX] -n [NUM]\n", argv[0]);
    fprintf(stderr, "Try '%s --help' for more information.\n", argv[0]);
    exit(EXIT_FAILURE);
}

int
main(int argc, char** argv, char** envp)
{
    int opt;
    FILE* output_fp;
    char* output_path;
    char* prefix_arg;
    unsigned int i, num, lox, loy, hix, hiy, x_dis_max, y_dis_max, x, y;
    double actskew, minskew, maxskew, ptarg;
    long unsigned size, targsize;

    long unsigned actwidth, actheight, actsize;

    std::string prefix;

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

    num = 0;
    prefix_arg = NULL;
    output_path = NULL;
    output_fp = NULL;
    minskew = maxskew = ptarg = 0.0;

    actwidth = actheight = actsize = 0;

    lox = hix = loy = hiy = 0;

    while ((opt = getopt(argc, argv, "x:n:f:g:h:i:")) != -1)
    {
        switch (opt)
        {
            case 'x':
                prefix_arg = optarg;
                break;
            case 'n':
                num = strtoul(optarg, NULL, 10);
                break;
            case 'f':
                output_path = optarg;
                break;
            case 'g':
                minskew = strtod(optarg, NULL);
                break;
            case 'h':
                maxskew = strtod(optarg, NULL);
                break;
            case 'i':
                ptarg = strtod(optarg, NULL);
                break;
            default:
                exit_fprintf_usage(argv);
                break;
        }
    }

    if (prefix_arg == NULL)
    {
        exit_fprintf_usage(argv);
    }
    else
    {
        prefix = std::string(prefix_arg);
    }
    if (num == 0)
    {
        exit_fprintf_usage(argv);
    }
    if (output_path == NULL)
    {
        output_fp = stdout;
    }
    else
    {
        output_fp = fopen(output_path, "w");
    }

    readcsv_get_uint(fopen((prefix + ".min_x").c_str(), "r"), &lox);
    readcsv_get_uint(fopen((prefix + ".max_x").c_str(), "r"), &hix);
    readcsv_get_uint(fopen((prefix + ".min_y").c_str(), "r"), &loy);
    readcsv_get_uint(fopen((prefix + ".max_y").c_str(), "r"), &hiy);
    hix -= 1;
    hiy -= 1;
    size = (hix - lox) * (hiy - loy);
    targsize = floor(size * ptarg);

    // RNG stuff
    std::random_device rdev;
    std::mt19937 rgen(rdev());
    std::uniform_real_distribution<double> skew_dis(minskew, maxskew);

    for (i = 0; i < num; i++)
    {
        actskew = skew_dis(rgen);
        if (actskew >= 0.0)
        {
            actwidth = sqrt((actskew + 1)*targsize);
            actheight = targsize / actwidth;
        }
        else
        {
            actheight = sqrt((fabs(actskew) + 1)*targsize);
            actwidth = targsize / actheight;
        }
        actsize = actwidth * actheight;

        x_dis_max = hix - actwidth;
        y_dis_max = hiy - actheight;
        if (actwidth  > (hix - lox)) x_dis_max = lox;
        if (actheight > (hiy - loy)) y_dis_max = loy;
        std::uniform_int_distribution<unsigned int> x_dis(lox, x_dis_max);
        std::uniform_int_distribution<unsigned int> y_dis(loy, y_dis_max);
        x = x_dis(rgen);
        y = y_dis(rgen);

        /*
        fprintf(output_fp, "x: %u-%u, y: %u-%u, skew: %lf-%lf, ptarg: %lf\n",
                lox, hix, loy, hiy, minskew, maxskew, ptarg);
        fprintf(output_fp, "size: %lu, targsize: %lu\n", size, targsize);
        fprintf(output_fp, "w: %6lu, h: %6lu, actsize: %6lu, actskew: %lf\n",
                actwidth, actheight, actsize, actskew);
        fprintf(output_fp, "x: %u, y: %u\n", x, y);
        */
        fprintf(output_fp, "%u %u %lu %lu", x, y, x+actwidth,
                y+actheight);
        //fprintf(output_fp, " %lf", actsize / (double)size);
        fprintf(output_fp, "\n");
    }

    exit(EXIT_SUCCESS);
}
