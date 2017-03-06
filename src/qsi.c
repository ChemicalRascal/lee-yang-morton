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

void
qsi_set_u(qsiseq* seq, long unsigned int u)
{
    /* Only really its own function in case this, later, involves doing more
     * (such as if existing sequences would need to be extensively retooled
     * after changing u).
     */
    seq->u = u;
}

void
qsi_set_n(qsiseq* seq, long unsigned int n)
{
    /* Only really its own function in case this, later, involves doing more
     * (such as if existing sequences would need to be extensively retooled
     * after changing n).
     */
    seq->n = n;
}

/* Where n is zero-indexed.
 */
long unsigned int
qsi_get_upper(qsiseq* seq, long unsigned int n)
{
    long unsigned int sum, ret, i;
    unsigned int hi_seq_index;
    hi_seq_index = 0;
    sum = 0;

    for (i = 0; i <= n; i++)
    {
        ret = read_unary_as_uint(seq->hi, &hi_seq_index);
        if (ret == UINT_MAX)
        {
            break;
        }
        else
        {
            sum += ret;
        }
    }

    return sum;
}

long unsigned int
qsi_get_final_upper(qsiseq* seq)
{
    /* If qsi_get_upper's implementation changes, this might not work. But for
     * now, it does! Never not explot integer overflow.
     */
    return qsi_get_upper(seq, ULONG_MAX);
}
