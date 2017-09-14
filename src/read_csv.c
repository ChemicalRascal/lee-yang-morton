/* vim: set et sts=4 sw=4 cc=80 tw=80: */

/*******************************************
 *
 *          File header goes here?
 *              In theory?
 *
 *******************************************/

#include "read_csv.h"

#include <stdio.h>
#include <ctype.h>
#include <string.h>

#define UINT_FORMAT "%u"
#define LUINT_FORMAT "%lu"

int
readcsv_peekc(FILE* file)
{
    int val;
    val = fgetc(file);
    if (val != EOF)
    {
        ungetc(val, file);
    }
    return val;
}

/* Please, please, please malloc dest.
 *
 * Returns EOF if EOF gets hit.
 */
int
readcsv_get_arbint(FILE* file, void* dest, const char* format)
{
    int peek_val;
    while (1 == 1)
    {
        peek_val = readcsv_peekc(file);
        if (peek_val == EOF)
        {
            return EOF;
        }

        if (!(isdigit(peek_val)))
        {
            /* Go through stuff that aren't numbers. */
            fgetc(file);
            continue;
        }
        else
        {
            /* Got an integer */
            if (strcmp(format, UINT_FORMAT) == 0)
            {
                peek_val = fscanf(file, format, (unsigned int*) dest);
            }
            if (strcmp(format, LUINT_FORMAT) == 0)
            {
                peek_val = fscanf(file, format, (long unsigned int*) dest);
            }
            if (peek_val == 0)
            {
                return EOF;
            }
            return peek_val;
        }
    }
}

int
readcsv_get_uint(FILE* file, unsigned int* dest)
{
    return readcsv_get_arbint(file, (void*) dest, UINT_FORMAT);
}

int
readcsv_get_luint(FILE* file, long unsigned int* dest)
{
    return readcsv_get_arbint(file, (void*) dest, LUINT_FORMAT);
}

int
readcsv_get_double(FILE* file, double* dest)
{
    int peek_val;
    while (1 == 1)
    {
        peek_val = readcsv_peekc(file);
        if (peek_val == EOF)
        {
            return EOF;
        }

        if (!(isdigit(peek_val) || peek_val == '-'))
        {
            /* Go through stuff that aren't numbers. Or -, as floats can be
             * negative, y'know.
             */
            fgetc(file);
            continue;
        }
        else
        {
            /* Got digit */
            peek_val = fscanf(file, "%lf", dest);
            if (peek_val == 0)
            {
                return EOF;
            }
            return peek_val;
        }
    }
}
