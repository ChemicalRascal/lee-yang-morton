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
#include <unistd.h>
#include <string.h>

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
    int depth, opt, print_len_mode;
    FILE* output_fp;
    char* output_path;
    long unsigned int max, lox, loy, hix, hiy, len;

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

    depth = 0;
    print_len_mode = 1;
    output_path = NULL;
    output_fp = NULL;

    while ((opt = getopt(argc, argv, "d:f:l")) != -1)
    {
        switch (opt)
        {
            case 'd':
                depth = atoi(optarg);
                break;
            case 'f':
                output_path = optarg;
                break;
            case 'l':
                print_len_mode = 1;
                break;
            default:
                exit_fprintf_usage(argv);
                break;
        }
    }

    if (depth <= 0)
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

    max = (1 << depth);
    len = 0;

    for (lox = 0; lox < max; lox++)
    {
        for (hix = lox; hix < max; hix++)
        {
            for (loy = 0; loy < max; loy++)
            {
                for (hiy = loy; hiy < max; hiy++)
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
