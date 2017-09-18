If you so much as glance sideways at the structs, update the appropriate
serialize magic numbers.

`make` to make. `make avx2` if your processor has AVX2 support to
activate ludicrous speed.

Building is dependent on Simon Gog's "Succinct Data Structure Library",
avaliable at https://github.com/simongog/sdsl-lite -- either install it
somewhere that your system expects it to be, or edit the Makefile to
point g++ at the right directories.

Compilation options:

* `BITSEQ_MINIMAL_WRITE` -- Make bitseq truncate away bits that were
allocated but weren't actually set prior to writing to disk.
