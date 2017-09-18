/* vim: set et sts=4 sw=4 cc=80 tw=80: */

/*******************************************
 *
 * This used to be an actual thing, now it's
 * just a wrapper around sdsl's bitvectors
 *
 *******************************************/

#include "bitseq.hpp"

#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <assert.h>

#include <stdint.h>
#include <endian.h>

/* Change these whenever formats or structs change. One might suggest
 * something that looks like the current date/time.
 */
#define BITSEQ_SERIALIZE_MAGIC_NUMBER 201707040000L

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
    b = (bitseq*)malloc(sizeof(bitseq));
    assert(b != NULL);
    b->vec = new sdsl::bit_vector(1);
    b->len = 0;
    return b;
}

void
free_bitseq(bitseq* seq)
{
    if (seq != NULL)
    {
        delete seq->vec;
    }
    free(seq);
}

void
double_bitseq(bitseq* seq)
{
    long unsigned int newsize;
    newsize = seq->len * 2;
    if (newsize == 0)
    {
        // This can technically happen if a bitseq is empty, is written, and
        // then read.
        newsize = 2;
    }
    seq->vec->resize(newsize);
}

/* Writes a bitseq to a given ostream, outfile.
 */
void
write_bitseq(bitseq* seq, std::ostream& outfile)
{
    long unsigned int magic = BITSEQ_SERIALIZE_MAGIC_NUMBER;
    outfile.write((char*)&magic, sizeof(long unsigned int));
    if (!outfile.good())
    {
        fprintf(stderr, "ERROR: write_bitseq: magic write failure.\n");
        return;
    }
#ifdef BITSEQ_MINIMAL_WRITE
    //Truncate the vector before writing.
    seq->vec->resize(seq->len);
#endif /* BITSEQ_MINIMAL_WRITE */
    seq->vec->serialize(outfile);
    if (!outfile.good())
    {
        fprintf(stderr, "ERROR: write_bitseq: seq->vec write failure.\n");
        return;
    }
    return;
}

/* Reads a bitseq from a given istream, infile.
 */
bitseq*
read_bitseq(std::istream& infile)
{
    long unsigned int magic;
    bitseq* seq;

    infile.read((char*)&magic, sizeof(long unsigned int));
    if (!infile.good())
    {
        fprintf(stderr, "ERROR: read_bitseq: magic read failure.\n");
        return NULL;
    }
    if (magic != BITSEQ_SERIALIZE_MAGIC_NUMBER)
    {
        fprintf(stderr,
                "ERROR: read_bitseq: magic num %lu expected, %lu found.\n",
                BITSEQ_SERIALIZE_MAGIC_NUMBER, magic);
        return NULL;
    }

    seq = (bitseq*)malloc(sizeof(bitseq));
    assert(seq != NULL);
    seq->vec = new sdsl::bit_vector();
    seq->vec->load(infile);
    if (!infile.good())
    {
        fprintf(stderr, "ERROR: read_bitseq: sec->vec read failure.\n");
        free(seq);
        return NULL;
    }
    return seq;
}

void
insert_bit(bitseq* seq, unsigned int index, unsigned char bit)
{
    /* Mask the input: We only want the least significant bit.
     */
    bit = bit & 1;
    if (seq->vec->size() <= index)
    {
        double_bitseq(seq);
    }
    (*(seq->vec))[index] = bit;
    if (seq->len < index)
    {
        seq->len = index + 1;
    }
    return;
}

void
append_bit(bitseq* seq, unsigned char bit)
{
    bit = bit & 1;
    if (seq->vec->size() <= seq->len)
    {
        double_bitseq(seq);
    }
    (*(seq->vec))[seq->len] = bit;
    seq->len += 1;
    return;
}

/* Returns 2 if seq is null, has nothing in it, or if index is beyond the end
 * of the sequence.
 */
unsigned char
get_bit(bitseq* seq, unsigned int index)
{
    if (seq == NULL || seq->vec == NULL || seq->vec->size() <= index)
    {
        return 2;
    }

    return (*(seq->vec))[index];
}

void
pprint_bitseq(bitseq* seq)
{
    long unsigned int i;

    if (seq == NULL)
    {
        printf("NULL\n");
        return;
    }

    printf("%lu/%lu ", seq->len, seq->vec->capacity());

    for (i = 0; i < seq->len; i++)
    {
        printf("%u", get_bit(seq, i));
        if (i%8 == 7)
        {
            printf(" ");
        }
    }
    printf("\n");
    return;
}

/* Appends the lowest n bits of a to seq. Unfortunately, does so in lsb->msb
 * order, due to how set_int is implemented in sdsl::int_vector.
 */
void
append_luint_bits_low(bitseq* seq, long unsigned int a, unsigned int n)
{
    long unsigned int mask = (1<<n)-1;
    a &= mask;
    seq->vec->set_int(seq->len, a, n);
    seq->len += n;
    //append_bit(seq, (unsigned char)((a>>i)&1));
}

/* Reads a chunk of bits as a luint, in lsb->msb order because that's how
 * sdsl::int_vector works.
 *
 * i: Index to start reading from.
 * n: Number of bits to read as luint.
 */
long unsigned int
get_luint(bitseq* seq, long unsigned int i, unsigned int n)
{
    return seq->vec->get_int(i, n);
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
read_unary_as_uint(bitseq* seq, long unsigned int* index)
{
    long unsigned int count = 0;
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
