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

#include <unistd.h>
#include <string.h>

#define fprintf_if_eq(a, b, fp, args...) {if((a)==(b)){fprintf((fp), args);}}
#define q_fprintf(fp, args...) fprintf_if_eq(global_quiet_mode, 0, fp, args)

int global_quiet_mode;
char* global_input_path;
char* global_tree_path;

/* Currently, data is assigned to *every* node.
 */
n_qtree*
read_qtree(FILE* fp, void* data, char* input_path)
{
    n_qtree* tree;
    int ret_val;
    unsigned int uint_read, uint_read_2;

    fprintf_if_eq(input_path, NULL, stdout, "Enter tree depth: ");
    ret_val = readcsv_get_uint(stdin, &uint_read);
    if (ret_val == EOF)
    {
        return NULL;
    }
    tree = new_qtree(uint_read);

    fprintf_if_eq(input_path, NULL, stdout,
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
    int opt;
    int search_mode, build_mode;
    FILE* input_fp;
    FILE* tree_fp;

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
    build_mode = 0;
    global_input_path = NULL;
    global_tree_path = NULL;
    input_fp = NULL;
    tree_fp = NULL;

    while ((opt = getopt(argc, argv, "bsqf:t:")) != -1)
    {
        switch (opt)
        {
            case 'b':
                build_mode = 1;
                break;
            case 'q':
                global_quiet_mode = 1;
                break;
            case 'f':
                global_input_path = optarg;
                break;
            case 't':
                global_tree_path = optarg;
                break;
            default:
                exit_fprintf_usage(argv);
                break;
        }
    }

    printf("q: %d, b: %d, gip: %s, gtp: %s\n", global_quiet_mode,
            build_mode, global_input_path, global_tree_path);

    exit(EXIT_SUCCESS);
}
