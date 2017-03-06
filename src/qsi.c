/* vim: set et sts=4 sw=4 cc=80 tw=80: */

/*******************************************
 *
 *          File header goes here?
 *              In theory?
 *
 *******************************************/

#include "qsi.h"

#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <assert.h>

qsiseq*
new_qsiseq()
{
    qsiseq* q;
    q = malloc(sizeof(qsiseq));
    q->hi = new_bitseq();
    q->lo = new_bitseq();
    return q;
}
