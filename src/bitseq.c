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
