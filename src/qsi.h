/* vim: set et sts=4 sw=4 cc=80 tw=80: */

/*******************************************
 *
 *          File header goes here?
 *              In theory?
 *
 *******************************************/

#ifndef QSI_H
#define QSI_H

#include "bitseq.h"

typedef struct qsiseq_s
{
    bitseq*             hi; // High-bits sequence
    bitseq*             lo; // Low-bits sequence
    long unsigned int   u;  // Upper bound
    long unsigned int   n;  // (Expected) number of elements
} qsiseq;

qsiseq* new_qsiseq();

void qsi_set_u(qsiseq*, long unsigned int);
void qsi_set_n(qsiseq*, long unsigned int);

long unsigned int qsi_get_upper(qsiseq*, long unsigned int);
long unsigned int qsi_get_final_upper(qsiseq*);

#endif /* QSI_H */
