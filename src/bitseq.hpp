/* vim: set et sts=4 sw=4 cc=80 tw=80: */

/*******************************************
 *
 *          File header goes here?
 *              In theory?
 *
 *******************************************/

#ifndef BITSEQ_H
#define BITSEQ_H

#include <stdio.h>
#include <iostream>

typedef struct bitseq_s
{
    unsigned int    length;     // In bits
    unsigned int    alloc_size; // In bytes
    unsigned char*  seq;
} bitseq;

bitseq* new_bitseq();
void free_bitseq(bitseq*);

void write_bitseq(bitseq*, std::ostream&);
bitseq* read_bitseq(std::istream&);

void insert_bit(bitseq*, unsigned int, unsigned char);
void append_bit(bitseq*, unsigned char);

void append_luint_bits_low(bitseq*, long unsigned int, unsigned int);

void append_uint_in_unary(bitseq*, unsigned int);

unsigned char get_bit(bitseq*, unsigned int);

unsigned int read_unary_as_uint(bitseq*, long unsigned int*);

bitseq* new_bitseq_from_ptr(void*, unsigned int);
bitseq* new_bitseq_from_int(int);
bitseq* new_bitseq_from_uint(unsigned int);

bitseq* weave_bits(void*, void*, unsigned int);
bitseq* weave_ints(int, int);
bitseq* weave_uints(unsigned int, unsigned int);

long unsigned int get_as_luint_ljust(bitseq*);
long unsigned int get_as_luint_rjust(bitseq*);
unsigned int get_as_uint_ljust(bitseq*);
unsigned int get_as_uint_rjust(bitseq*);

long unsigned int weave_uints_to_luint(unsigned int, unsigned int);
void unweave_luint_to_uints(long unsigned int, unsigned int*, unsigned int*);

void pprint_bitseq(bitseq*);


unsigned char get_bit_void_ptr(void*, unsigned int);
void htobe(void*, unsigned int);

#endif /* BITSEQ_H */
