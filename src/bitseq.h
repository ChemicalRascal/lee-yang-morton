/* vim: set et sts=4 sw=4 cc=80 tw=80: */

/*******************************************
 *
 *          File header goes here?
 *              In theory?
 *
 *******************************************/

typedef struct bitseq_s
{
    unsigned int    length;     // In bits
    unsigned int    alloc_size; // In bytes
    unsigned char*  seq;
} bitseq;

bitseq* new_bitseq();

void insert_bit(bitseq*, unsigned int, unsigned char);
void append_bit(bitseq*, unsigned char);

unsigned char get_bit(bitseq*, unsigned int);

void pprint_bitseq(bitseq*);
