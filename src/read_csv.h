/* vim: set et sts=4 sw=4 cc=80 tw=80: */

/*******************************************
 *
 *          File header goes here?
 *              In theory?
 *
 *******************************************/

#ifndef READ_CSV_H
#define READ_CSV_H

#include <stdio.h>

int readcsv_get_uint(FILE*, unsigned int*);
int readcsv_get_luint(FILE*, long unsigned int*);

#endif /* READ_CSV_H */
