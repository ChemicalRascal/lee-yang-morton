/* vim: set et sts=4 sw=4 cc=80 tw=80: */

/*******************************************
 *
 *  Main testbench program for leeyang.h,
 *      and associated libraries.
 *
 *******************************************/

#include <stdlib.h>

#include <fstream>

#include "bitseq.hpp"

int
main(int argc, char** argv, char** envp)
{
    bitseq* seq = new_bitseq();

    std::ofstream outfile ("temp.file", std::ofstream::binary);

    append_bit(seq, 0);
    append_bit(seq, 1);
    append_bit(seq, 0);
    append_bit(seq, 0);
    append_bit(seq, 1);

    write_bitseq(seq, outfile);

    outfile.put('b');

    outfile.close();

    exit(EXIT_SUCCESS);
}
