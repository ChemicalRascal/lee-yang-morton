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

#include "bitseq.h"
#include "qsi.h"
#include "leeyang.h"
#include "read_csv.h"

#include <limits.h>

#include <unistd.h>
#include <string.h>

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
    int opt, build_mode;
    FILE* input_fp;
    FILE* tree_fp;
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

    if (global_tree_path == NULL)
    {
        exit_fprintf_usage(argv);
    }

    input_fp = fopen(global_input_path, "r");
    tree_fp = fopen(global_tree_path, (build_mode==1)?"wb":"rb");

    if (build_mode == 1)
    {
        n_qtree* tree;
        int junk_data = 1;

        tree = read_qtree(input_fp, &junk_data);
        link_nodes_morton(tree);
        seq = qsiseq_from_n_qtree(tree);
        free_qtree(tree, 1);
        write_qsiseq(seq, tree_fp);
        free_qsiseq(seq);
        exit(EXIT_SUCCESS);
    }

    if (build_mode == 0)
    {
        seq = read_qsiseq(tree_fp);
    }

    /*
    printf("q: %d, b: %d, gip: %s, gtp: %s\n", global_quiet_mode,
            build_mode, global_input_path, global_tree_path);
            */

    exit(EXIT_SUCCESS);
}
