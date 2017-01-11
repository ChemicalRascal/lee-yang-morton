MAKE	= gcc
MKFLAGS = -Wall
RM	= rm
NAIVE	= naive_quadtree.c
PROG	= qtree_testbench
VGRIND	= valgrind
VGFLAG	= --leak-check=yes
VGCFLAG = -g -O0

.PHONY: default
default: all

.PHONY: all
all:
	$(MAKE) $(MKFLAGS) -C $(NAIVE) -o $(PROG)

.PHONY: val_build
val_build:
	$(MAKE) $(MKFLAGS) $(VGCFLAG) -C $(NAIVE) -o $(PROG)

.PHONY: val
val:	val_build
	$(VGRIND) $(VGFLAG) ./$(PROG)

.PHONY: naive
naive:
	$(MAKE) $(MKFLAGS) -C $(NAIVE) -o $(PROG)

.PHONY: clean
clean:
	$(RM) $(PROG) vgcore.*
