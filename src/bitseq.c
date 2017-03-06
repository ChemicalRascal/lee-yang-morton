/* vim: set et sts=4 sw=4 cc=80 tw=80: */

/*******************************************
 *
 *          File header goes here?
 *              In theory?
 *
 *******************************************/

#include "bitseq.h"

#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <assert.h>

#include <stdint.h>
#include <endian.h>

/* It physically pains me to do this
 *
 * Endian conversion function abstraction. For handling nonsense and whatever,
 * because I guess I pass around too many void pointers. But, well, we've come
 * this far.
 */

void
htobe(void* p, unsigned int len)
{
    if (len == 16)
    {
        uint16_t* work = (uint16_t*) p;
        *work = htobe16(*work);
        return;
    }
    else if (len == 32)
    {
        uint32_t* work = (uint32_t*) p;
        *work = htobe32(*work);
        return;
    }
    else if (len == 64)
    {
        uint64_t* work = (uint64_t*) p;
        *work = htobe64(*work);
        return;
    }
}

void
htole(void* p, unsigned int len)
{
    if (len == 16)
    {
        uint16_t* work = (uint16_t*) p;
        *work = htole16(*work);
        return;
    }
    else if (len == 32)
    {
        uint32_t* work = (uint32_t*) p;
        *work = htole32(*work);
        return;
    }
    else if (len == 64)
    {
        uint64_t* work = (uint64_t*) p;
        *work = htole64(*work);
        return;
    }
}

void
betoh(void *p, unsigned int len)
{
    if (len == 16)
    {
        uint16_t* work = (uint16_t*) p;
        *work = be16toh(*work);
        return;
    }
    else if (len == 32)
    {
        uint32_t* work = (uint32_t*) p;
        *work = be32toh(*work);
        return;
    }
    else if (len == 64)
    {
        uint64_t* work = (uint64_t*) p;
        *work = be64toh(*work);
        return;
    }
}

void
letoh(void *p, unsigned int len)
{
    if (len == 16)
    {
        uint16_t* work = (uint16_t*) p;
        *work = le16toh(*work);
        return;
    }
    else if (len == 32)
    {
        uint32_t* work = (uint32_t*) p;
        *work = le32toh(*work);
        return;
    }
    else if (len == 64)
    {
        uint64_t* work = (uint64_t*) p;
        *work = le64toh(*work);
        return;
    }
}

unsigned char get_bit_void_ptr(void*, unsigned int);

bitseq*
new_bitseq()
{
    bitseq* b;
    b = malloc(sizeof(bitseq));
    assert(b != NULL);
    b->length       = 0;
    b->alloc_size   = 0;
    b->seq          = NULL;
    return b;
}

void
free_bitseq(bitseq* seq)
{
    if (seq != NULL)
    {
        free(seq->seq);
    }
    free(seq);
}

/* Library worker function.
 */
void
realloc_bitseq(bitseq* seq, unsigned int newsize)
{
    unsigned int i;

    if (seq->alloc_size < newsize)
    {
        seq->seq = realloc(seq->seq, newsize);
        assert(seq->seq != NULL);

        /* Set all the bytes out to the new size out to 0.
         */
        for (i = seq->alloc_size; i < newsize; i++)
        {
            seq->seq[i] = 0;
        }
        seq->alloc_size = newsize;
    }

    return;
}

void
insert_bit(bitseq* seq, unsigned int index, unsigned char bit)
{
    unsigned int    newsize = 0;
    unsigned int    offset;
    unsigned char*  byte;

    /* Mask the input: We only want the least significant bit.
     */
    bit = bit & 1;

    // Allocate more space to the sequence
    if ((index/CHAR_BIT)+1 > seq->alloc_size)
    {
        /* Gotta have more bytes in our bits
         *
         * TODO: Shouldn't there be overflow checks here or something?
         */
        newsize = 2 * seq->alloc_size;
        if ((index/CHAR_BIT)+1 >= newsize)
        {
            newsize = ((index/CHAR_BIT)+1)*2;
        }

        realloc_bitseq(seq, newsize * sizeof(char));
    }

    // Get a pointer to the exact byte we want.
    byte = &(seq->seq[index/CHAR_BIT]);
    offset = index % CHAR_BIT;

    if (bit == 1)
    {
        /* bit = 1 is a simple case. R-shift, then just bitwise-or it.
         */
        bit = bit << (CHAR_BIT - offset - 1);
        *byte = *byte | bit;
    }
    else if (bit == 0)
    {
        /* If bit = 0, replace it with "inverted" 1 (via subtraction of an
         * r-shifted 1) and use &, not |?
         *
         * I really wish there was a cleaner way to do this. Or, at least, a
         * cleaner way to do this that I was aware of.
         */
        bit = UCHAR_MAX - (1 << (CHAR_BIT - offset - 1));
        *byte = *byte & bit;
    }
    else
    {
        /* Something has gone horribly wrong. So return here, before
         * seq->length is updated, as we haven't changed anything in
         * the sequence at this point.
         */
        return;
    }

    if (seq->length <= index)
    {
        seq->length = index + 1;
    }
    return;
}

/* The nice thing about writing insert_bit() first is that append_bit()
 * is really easy!
 */
void
append_bit(bitseq* seq, unsigned char bit)
{
    insert_bit(seq, seq->length, bit);
    return;
}

/* Returns 2 if seq is null, has nothing in it, or if index is beyond the end
 * of the sequence.
 */
unsigned char
get_bit(bitseq* seq, unsigned int index)
{
    if (seq == NULL || seq->seq == NULL || seq->length <= index)
    {
        return 2;
    }

    return get_bit_void_ptr((void*) seq->seq, index);
}

/* Library worker. Assumes everything has been checked.
 */
unsigned char
get_bit_void_ptr(void* ptr, unsigned int index)
{
    unsigned char   bit;
    unsigned char*  data = (unsigned char*) ptr;

    bit = data[index/CHAR_BIT];
    index = index % CHAR_BIT;
    bit = (bit >> (CHAR_BIT - index - 1)) & 1;
    return bit;
}

/* Treats a as a char array. len is the length to be copied in, in bits.
 */
bitseq*
new_bitseq_from_ptr(void* a, unsigned int len)
{
    bitseq* seq;
    unsigned int i;
    unsigned char* a_array = a;
    unsigned char c = 0;

    seq = new_bitseq();
    realloc_bitseq(seq, len);

    for (i = 0; i + CHAR_BIT <= len; i++)
    {
        seq->seq[i] = a_array[i];
    }
    if (len % CHAR_BIT != 0)
    {
        /* Handle the remaining partial byte. */
        c = a_array[len/CHAR_BIT];
        c >>= len % CHAR_BIT;
        c <<= len % CHAR_BIT;
        /* And by the magic of bitshifts, the final len % CHAR_BIT bits
         * of c are all 0.
         */
        seq->seq[len/CHAR_BIT] = c;
    }
    seq->length = len;

    return seq;
}

bitseq*
new_bitseq_from_int(int a)
{
    htobe((void*) &a, sizeof(int) * CHAR_BIT);
    return new_bitseq_from_ptr((void*) &a, sizeof(int) * CHAR_BIT);
}

bitseq*
new_bitseq_from_uint(unsigned int a)
{
    htobe((void*) &a, sizeof(unsigned int) * CHAR_BIT);
    return new_bitseq_from_ptr((void*) &a, sizeof(unsigned int) * CHAR_BIT);
}

/* Treating a and b as existing sequences, "weaves" the bits together:
 * The most significant of a, then the most significant of b, and then so on.
 * 'len' is the number of bits from each to weave.
 */
bitseq*
weave_bits(void* a, void* b, unsigned int len)
{
    unsigned int    i;
    unsigned char   bit;

    bitseq* seq;

    seq = new_bitseq();
    realloc_bitseq(seq, len * 2);

    for (i = 0; i < len; i++)
    {
        bit = get_bit_void_ptr(a, i);
        if (bit)
        {
            append_bit(seq, bit);
        }
        else
        {
            /* We can cheat when bit = 0, as we know realloc_bitseq()
             * initialises everything to 0. And we won't overrun the sequence,
             * because we've already set the length appropriately.
             */
            seq->length += 1;
        }
        bit = get_bit_void_ptr(b, i);
        if (bit)
        {
            append_bit(seq, bit);
        }
        else
        {
            seq->length += 1;
        }
    }

    return seq;
}

/* Helper function.
 */
bitseq*
weave_ints(int a, int b)
{
    /* While htob and such mutate what they're pointed at (as opposed to the
     * endian.h functions, which return values), these integers are on the
     * weave_ints stack so htobe and such won't mutate whatever is on the
     * caller's stack (or elsewhere).
     */
    htobe((void*) &a, sizeof(int) * CHAR_BIT);
    htobe((void*) &b, sizeof(int) * CHAR_BIT);
    return weave_bits((void*) &a, (void*) &b, sizeof(int) * CHAR_BIT);
}

/* Helper function. Causes the unsigned ints to be weaved together, in
 * big endian order.
 */
bitseq*
weave_uints(unsigned int a, unsigned int b)
{
    htobe((void*) &a, sizeof(unsigned int) * CHAR_BIT);
    htobe((void*) &b, sizeof(unsigned int) * CHAR_BIT);
    return weave_bits((void*) &a, (void*) &b, sizeof(unsigned int) * CHAR_BIT);
}

/* TODO: Make these not effectively error-out in the case of the sequence not
 * being long enough.
*/
long unsigned int
get_as_luint_ljust(bitseq* seq)
{
    long unsigned int a;

    if (seq == NULL || seq->seq == NULL
            || seq->length < sizeof(long unsigned int) * CHAR_BIT)
    {
        // TODO: Replace with making a new sequence and such
        return 0;
    }

    a = *(long unsigned int*) (seq->seq);
    betoh((void*) &a, sizeof(long unsigned int) * CHAR_BIT);
    return a;
}

long unsigned int
get_as_luint_rjust(bitseq* seq)
{
    long unsigned int a;

    if (seq == NULL || seq->seq == NULL
            || seq->length < sizeof(long unsigned int) * CHAR_BIT)
    {
        // TODO: Replace with making a new sequence and such
        return 0;
    }
    if (seq->length % CHAR_BIT == 0)
    {
        a = *(long unsigned int*) ((seq->seq) + ((seq->length/CHAR_BIT -
                    sizeof(long unsigned int))));
        betoh((void*) &a, sizeof(long unsigned int) * CHAR_BIT);
        return a;
    }
    else
    {
        // TODO: Replace with, somehow, bitshifting the array, or
        // something.
        return 0;
    }
}

long unsigned int
weave_uints_to_luint(unsigned int a, unsigned int b)
{
    bitseq* seq = weave_uints(a, b);
    long unsigned int r = get_as_luint_rjust(seq);
    free_bitseq(seq);
    return r;
}

/* TODO: Given that we can do this, it'd probably be... smarter to use a similar
 * method for weaving.
 */
void
unweave_luint_to_uints(long unsigned int x, unsigned int* a, unsigned int* b)
{
    unsigned int i;
    *a = 0;
    *b = 0;

    for (i = 0; i < (CHAR_BIT * sizeof(long unsigned int)); i++)
    {
        if (i%2 == 0)
        {
            *a <<= 1;
            *a += (x >> ((CHAR_BIT * sizeof(long unsigned int)) - i - 1))&1;
        }
        else
        {
            *b <<= 1;
            *b += (x >> ((CHAR_BIT * sizeof(long unsigned int)) - i - 1))&1;
        }
    }
}

unsigned int
get_as_uint_ljust(bitseq* seq)
{
    unsigned int a;

    if (seq == NULL || seq->seq == NULL
            || seq->length < sizeof(unsigned int) * CHAR_BIT)
    {
        // TODO: Replace with making a new sequence and such
        return 0;
    }

    a = *(unsigned int*) (seq->seq);
    betoh((void*) &a, sizeof(unsigned int) * CHAR_BIT);
    return a;
}

unsigned int
get_as_uint_rjust(bitseq* seq)
{
    unsigned int a;

    if (seq == NULL || seq->seq == NULL
            || seq->length < sizeof(unsigned int) * CHAR_BIT)
    {
        // TODO: Replace with making a new sequence and such
        return 0;
    }
    if (seq->length % CHAR_BIT == 0)
    {
        a = *(unsigned int*) ((seq->seq) + ((seq->length/CHAR_BIT -
                    sizeof(unsigned int))));
        betoh((void*) &a, sizeof(unsigned int) * CHAR_BIT);
        return a;
    }
    else
    {
        // TODO: Replace with copying and bitshifting the array? This is overly
        // painful.
        return 0;
    }
}

void
pprint_bitseq(bitseq* seq)
{
    unsigned int mask;
    unsigned int i;

    for (i = 0; i < (seq->length/CHAR_BIT); i++)
    {
        mask = 1 << (CHAR_BIT - 1);
        while (mask)
        {
            /* mask will eventually equal zero due to the repeated shift
             */
            printf("%u", (mask & seq->seq[i]) ? 1 : 0 );
            mask >>= 1;
        }
        printf(" ");
    }
    mask = 1 << (CHAR_BIT - 1);
    for (i = 0; i < (seq->length % CHAR_BIT); i++)
    {
        printf("%u", (mask & seq->seq[seq->length/CHAR_BIT]) ? 1 : 0 );
        mask >>= 1;
    }
    printf("\n");
    return;
}

/* Appends the lowest n bits of a to seq.
 */
void
append_luint_bits_low(bitseq* seq, long unsigned int a, unsigned int n)
{
    long int i;
    for (i = n - 1; i >= 0; i--)
    {
        append_bit(seq, (unsigned char)((a>>i)&1));
    }
}

/* Appends a as a unary-encoded integer to seq.
 *
 * NB: Unary encoding is defined as 0^a1.
 */
void
append_uint_in_unary(bitseq* seq, unsigned int a)
{
    while (a > 0)
    {
        append_bit(seq, 0);
        a--;
    }
    append_bit(seq, 1);
}

/* Reads the unary code starting at position *index as an unsigned int.
 *
 * Also increments *index, thus it will index to the bit after the read unary
 * code, making successive reads (theoretically) easier.
 *
 * If index is NULL, the read starts at the start of the sequence.
 *
 * Returns UINT_MAX if *index is beyond the length of seq (in which case it
 * doesn't get incremented), or the read runs over the end of the sequence (in
 * which case it does).
 */
unsigned int
read_unary_as_uint(bitseq* seq, unsigned int* index)
{
    unsigned int count = 0;
    unsigned char r;

    if (index == NULL)
    {
        return read_unary_as_uint(seq, &count);
    }

    r = get_bit(seq, *index);
    while(r == 0)
    {
        *index = *index + 1;
        count++;
        r = get_bit(seq, *index);
    }
    if (r == 2)
    {
        return UINT_MAX;
    }

    *index = *index + 1;
    return count;
}
