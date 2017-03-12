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

#define QSI_INIT_PSUMS_LEN  10

qsipsums*
new_qsipsums()
{
    qsipsums* s;
    s = malloc(sizeof(qsipsums));
    assert(s != NULL);
    s->psums = calloc(QSI_INIT_PSUMS_LEN, sizeof(qsipsum));
    s->len  = 0;
    s->size = QSI_INIT_PSUMS_LEN;
    return s;
}

qsiseq*
new_qsiseq()
{
    qsiseq* q;
    q = malloc(sizeof(qsiseq));
    assert(q != NULL);
    q->hi = new_bitseq();
    q->lo = new_bitseq();
    q->hi_psums = new_qsipsums();
    q->len = 0;
    return q;
}

void
qsi_append_psum(qsiseq* seq, long unsigned int index, long unsigned int sum)
{
    qsipsums* sums = seq->hi_psums;
    if (sums->len >= sums->size)
    {
        if (sums->size == 0)
        {
            /* This shouldn't be possible, but whatever */
            sums->size = 1;
        }
        sums->psums = realloc(sums->psums, sizeof(qsipsum)*(sums->size)*2);
        assert(sums->psums != NULL);
        sums->size *= 2;
    }
    sums->psums[sums->len].index = index;
    sums->psums[sums->len].sum = sum;
    sums->len += 1;
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

/* max(0, floor(log(u/n)))
 */
unsigned int
qsi_lowbit_length(qsiseq* seq)
{
    unsigned int count = 0;
    long unsigned int q;

    if (seq->n == 0)
    {
        return 0;
    }
    q = seq->u/seq->n;

    /* This, admittedly, assumes that the log function specified in Vigna's
     * paper for this calculation is base 2, not a natural log.
     */
    while (q != 0)
    {
        q >>= 1;
        count += 1;
    }
    if (count != 0)
    {
        count -= 1;
    }

    return count;
}

/* Where i is zero-indexed.
 */
long unsigned int
qsi_get_upper(qsiseq* seq, long unsigned int i)
{
    long unsigned int sum, ret, j, hi_seq_index;
    hi_seq_index = 0;
    sum = 0;

    for (j = 0; j <= i; j++)
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

void
qsi_append(qsiseq* seq, long unsigned int a)
{
    unsigned int l = qsi_lowbit_length(seq);

    /* Low bits */
    if (l > 0)
    {
        append_luint_bits_low(seq->lo, a, qsi_lowbit_length(seq));
    }

    /* High bits */
    a >>= l;
    a -= qsi_get_final_upper(seq);
    append_uint_in_unary(seq->hi, a);

    seq->len += 1;
}

void
pprint_qsipsums(qsipsums* sums)
{
    long unsigned int i;
    for (i = 0; i < sums->len; i++)
    {
        printf("%lu:%lu, ", sums->psums[i].index, sums->psums[i].sum);
    }
    printf("\n");
}

void
pprint_qsiseq(qsiseq* seq)
{
    printf("hi: ");
    pprint_bitseq(seq->hi);
    printf("lo: ");
    pprint_bitseq(seq->lo);
    printf("hi_psums: ");
    pprint_qsipsums(seq->hi_psums);
}
