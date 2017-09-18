/* vim: set et sts=4 sw=4 cc=80 tw=80: */
/*******************************************
 *
 * Exhaustive query generator
 *
 *******************************************/

#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>

#include <string>

#include "read_csv.h"

void
exit_fprintf_help(char** argv)
{
    fprintf(stdout, "Usage: %s [OPTION]... -d DEPTH\n", argv[0]);
    fprintf(stdout, "Generate a set of exhaustive queries.\n");
    fprintf(stdout, "\n");
    fprintf(stdout, "  -d DEPTH must be provided.\n");
    fprintf(stdout, "\n");
    fprintf(stdout, "  -d DEPTH     define qtree depth to gen queries for\n");
    fprintf(stdout, "  -f FILE      write to FILE instead of stdout\n");
    fprintf(stdout, "  -l           write number of queries to stderr\n");
    exit(EXIT_SUCCESS);
}

void
exit_fprintf_usage(char** argv)
{
    /*
     */
    fprintf(stderr, "Usage: %s [OPTION]... -d [DEPTH]\n", argv[0]);
    fprintf(stderr, "Try '%s --help' for more information.\n", argv[0]);
    exit(EXIT_FAILURE);
}

int
main(int argc, char** argv, char** envp)
{
    int opt, print_len_mode;
    FILE* output_fp;
    char* output_path;
    char* prefix_arg;
    long unsigned int lox, loy, hix, hiy, len;
    unsigned int x_min, x_max, y_min, y_max;
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

    print_len_mode = 1;
    output_path = NULL;
    output_fp = NULL;

    while ((opt = getopt(argc, argv, "x:f:")) != -1)
    {
        switch (opt)
        {
            case 'x':
                prefix_arg = optarg;
                break;
            case 'f':
                output_path = optarg;
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
    if (output_path == NULL)
    {
        output_fp = stdout;
    }
    else
    {
        output_fp = fopen(output_path, "w");
    }

    readcsv_get_uint(fopen((prefix + ".min_x").c_str(), "r"), &x_min);
    readcsv_get_uint(fopen((prefix + ".max_x").c_str(), "r"), &x_max);
    readcsv_get_uint(fopen((prefix + ".min_y").c_str(), "r"), &y_min);
    readcsv_get_uint(fopen((prefix + ".max_y").c_str(), "r"), &y_max);

    for (lox = x_min; lox < x_max; lox++)
    {
        for (hix = lox; hix < x_max; hix++)
        {
            for (loy = y_min; loy < y_max; loy++)
            {
                for (hiy = loy; hiy < y_max; hiy++)
                {
                    fprintf(output_fp, "%lu %lu %lu %lu\n", lox, loy, hix,
                            hiy);
                    len += 1;
                }
            }
        }
    }

    if (print_len_mode == 1)
    {
        fprintf(stderr, "len: %lu\n", len);
    }

    exit(EXIT_SUCCESS);
}
