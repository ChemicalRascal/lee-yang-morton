/* vim: set et sts=4 sw=4 cc=80 tw=80: */

/*******************************************
 * morton.h -- in which bits are interlaced
 *             in a rapid fashion
 *
 * With thanks to M. Petri for his morton.hpp,
 * from which this is derived heavily from.
 *
 * You problably want to compile this with
 * -mavx2 under gcc. On other compilers, you're
 * after a flag that gives support for AVX2.
 *******************************************/

#ifndef MORTON_H
#define MORTON_H

#include <stdint.h>

void morton_PtoZ(uint64_t, uint64_t, uint64_t*);
void morton_ZtoP(uint64_t, uint64_t*, uint64_t*);

#endif /* MORTON_H */
