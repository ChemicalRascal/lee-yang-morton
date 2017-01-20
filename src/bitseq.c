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
    htobe((void*) &a, sizeof(unsigned int) * CHAR_BIT);
    htobe((void*) &b, sizeof(unsigned int) * CHAR_BIT);
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
        return 0;
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
