MAKE	= gcc
MKFLAGS = -Wall
RM	= rm
NAIVE	= naive_quadtree.c

.PHONY: default
default: all

.PHONY: all
all:
	$(MAKE) $(MKFLAGS) -C $(NAIVE)

.PHONY: naive
naive:
	$(MAKE) $(MKFLAGS) -C $(NAIVE)

.PHONY: clean
clean:
	$(RM) a.out
