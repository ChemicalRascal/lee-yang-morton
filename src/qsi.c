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
#define QSI_DEFAULT_Q       2


unsigned int qsi_set_lowbit_length(qsiseq*);

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

void
free_qsipsums(qsipsums* psums)
{
    free(psums->psums);
    free(psums);
    return;
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
    q->len = 0L;
    q->max = 0L;
    q->tree_depth = 0L;
    q->q = QSI_DEFAULT_Q;
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

/* Assumes that seq->hi_psums are kosher.
 */
void
qsi_update_psums(qsiseq* seq)
{
    long unsigned int current_sum, current_index, i;

    if (seq->len/seq->q > seq->hi_psums->len)
    {
        if (seq->hi_psums->len == 0)
        {
            current_sum = 0;
            current_index = 0;
            /* Handle initial psum. */
            qsi_append_psum(seq, 0, 0);
        }
        else
        {
            current_sum = seq->hi_psums->psums[seq->hi_psums->len-1].sum;
            current_index = seq->hi_psums->psums[seq->hi_psums->len-1].index;
        }
    }

    while (seq->len/seq->q > seq->hi_psums->len)
    {
        for (i = 0; i < seq->q; i++)
        {
            current_sum += read_unary_as_uint(seq->hi, &current_index);
        }
        qsi_append_psum(seq, current_index, current_sum);
    }
}

void
qsi_set_u(qsiseq* seq, long unsigned int u)
{
    /* Only really its own function in case this, later, involves doing more
     * (such as if existing sequences would need to be extensively retooled
     * after changing u).
     */
    seq->u = u;
    qsi_set_lowbit_length(seq);
}

void
qsi_set_n(qsiseq* seq, long unsigned int n)
{
    /* Only really its own function in case this, later, involves doing more
     * (such as if existing sequences would need to be extensively retooled
     * after changing n).
     */
    seq->n = n;
    qsi_set_lowbit_length(seq);
}

/* max(0, floor(log(u/n)))
 *
 * Sanely handles situations where u and n are zero.
 */
unsigned int
qsi_set_lowbit_length(qsiseq* seq)
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

    seq->l = count;
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
    if (a > seq->max)
    {
        seq->max = a;
    }

    /* Low bits */
    if (seq->l > 0)
    {
        append_luint_bits_low(seq->lo, a, seq->l);
    }

    /* High bits */
    a >>= seq->l;
    a -= seq->final_upper;
    append_uint_in_unary(seq->hi, a);
    seq->final_upper += a;

    seq->len += 1;
}

/* Can't use glib's bsearch(), because we want a match or immediate lower
 * bound, not just a match.
 *
 * Returns the index of the highest lower bound on "target" in psums. Ergo:
 *      target = 4
 *      1 2 3 4 5 6 -> 3
 *      1 2 3 5 6 9 -> 2
 * Importantly, though, it returns the *lowest* index if there are multiple
 * instances of the target in the sequence, otherwise, the highest index if the
 * highest lower bound is not equal to the target:
 *      target = 4
 *      1 1 2 2 3 4 4 5 5 6 -> 5
 *      1 1 2 2 3 3 3 5 5 6 -> 6
 *
 * Returns ULONG_MAX if something goes wrong.
 */
long unsigned int
qsi_psum_bsearch(qsipsums* psums, long unsigned int target)
{
    long unsigned int lo, hi, mid;
    int comp;

    lo = 0;
    hi = psums->len;

    if (target < psums->psums[lo].sum || target > psums->psums[hi].sum)
    {
        fprintf(stdout, "qsi_psum_bsearch: Err 00\n");
        return ULONG_MAX;
    }

    while (lo < hi)
    {
        if (hi == lo + 1)
        {
            if (target < psums->psums[lo].sum)
            {
                fprintf(stdout, "qsi_psum_bsearch: Err 01\n");
                return ULONG_MAX;
            }
            if (target > psums->psums[hi].sum)
            {
                fprintf(stdout, "qsi_psum_bsearch: Err 02\n");
                return ULONG_MAX;
            }
            break;
        }
        /* Excessively complex calculation to avoid integer overflow.
         */
        mid = (lo/2) + (hi/2) + ((lo%2 + hi%2)/2);
        comp = target - psums->psums[mid].sum;
        if (comp < 0)
        {
             hi = mid;
             continue;
        }
        else if (comp > 0)
        {
             lo = mid;
             continue;
        }
        else if (comp == 0)
        {
            lo = mid;
            hi = mid;
            break;
        }
    }

    /* Rewind -- psums is strictly non-decreasing, but may not be strictly
     * monotonically increasing. We want the very lowest psum, if target is
     * equal to that psum.
     */
    if (target == psums->psums[lo].sum)
    {
        while (lo != 0 && (psums->psums[lo].sum == psums->psums[lo-1].sum))
        {
            lo--;
        }
    }
    return lo;
}

/* Returns target if target is in seq, or the lowest upper-bound on target if
 * target is not in seq.
 *
 * If state is not NULL, then it is set in order to facillitate the use of
 * qsi_get_next.
 *
 * Returns ULONG_MAX if something went wrong.
 *
 * NOTE: Behaviour in sequences with duplicates, when that duplicate is the
 * target, and especially when one of those duplicates falls on a "partial
 * sum", is undefined in regards to the eventual location of "state" -- it
 * may be positioned to read the second duplicate, it may not be.
 */
long unsigned int
qsi_get(qsiseq* seq, qsi_next_state* state, long unsigned int target)
{
    long unsigned int t_hi, psum_index, val;
    qsi_next_state local_state;

    if ((seq == NULL) || (target > seq->max))
    {
        return ULONG_MAX;
    }

    t_hi = target >> seq->l;
    psum_index = qsi_psum_bsearch(seq->hi_psums, t_hi);

    local_state.lo = psum_index * seq->q * seq->l;
    local_state.hi = seq->hi_psums->psums[psum_index].index;
    local_state.running_psum = seq->hi_psums->psums[psum_index].sum;
    val = qsi_get_next(seq, &local_state);

    while (val > target)
    {
        /* qsi_psum_bsearch doesn't consider the low bits. When considering
         * the low bits, we may find that we're actually one "psum bracket"
         * too far forward.
         *
         * Of course, it's easier to just use qsi_get_next in the loop, instead
         * of masking off the low bits from target and using that to compare.
         */
        if (psum_index == 0)
        {
            break;
        }
        psum_index -= 1;
        local_state.lo = psum_index * seq->q * seq->l;
        local_state.hi = seq->hi_psums->psums[psum_index].index;
        local_state.running_psum = seq->hi_psums->psums[psum_index].sum;
        val = qsi_get_next(seq, &local_state);
    }

    while (val < target)
    {
        val = qsi_get_next(seq, &local_state);
    }

    if (state != NULL)
    {
        state->lo = local_state.lo;
        state->hi = local_state.hi;
        state->running_psum = local_state.running_psum;
    }

    return val;
}

/* For a given next_state, "increments" the state and returns the luint that the
 * state pointed *to* (not what it points to after incrementing).
 *
 * Returns ULONG_MAX if something goes wrong.
 */
long unsigned int
qsi_get_next(qsiseq* seq, qsi_next_state* state)
{
    long unsigned int read_lo, read_hi, i;
    unsigned char bit;

    if (state == NULL || seq == NULL)
    {
        return ULONG_MAX;
    }

    read_lo = 0;
    for (i = 0; i < seq->l; i++)
    {
        bit = get_bit(seq->lo, state->lo + i);
        if (bit == 2)
        {
            return ULONG_MAX;
        }
        read_lo = (read_lo << 1) | bit;
    }

    i = state->hi;
    read_hi = read_unary_as_uint(seq->hi, &i);
    if (read_hi == UINT_MAX)
    {
        return ULONG_MAX;
    }
    state->running_psum += read_hi;
    read_hi = state->running_psum;
    state->hi = i;
    state->lo += seq->l;

    return ((read_hi << seq->l) | read_lo);
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
    printf("u: %lu, ", seq->u);
    printf("n: %lu, ", seq->n);
    printf("len: %lu, ", seq->len);
    printf("max: %lu, ", seq->max);
    printf("l: %u, ", seq->l);
    printf("q: %u, ", seq->q);
    printf("\n");
    printf("hi: ");
    pprint_bitseq(seq->hi);
    printf("lo: ");
    pprint_bitseq(seq->lo);
    printf("hi_psums: ");
    pprint_qsipsums(seq->hi_psums);
}
