CC  	= gcc
CFLAGS	= -Wall
RM	= rm
RMFLAGS	= -f

PROG	= qtree_testbench
DEPS	= src/bitseq.h src/qsi.h
OBJ	= bin/bitseq.o bin/qsi.o bin/qtree_testbench.o
CFILES	= src/bitseq.c src/qsi.c src/qtree_testbench.c

VGRIND	= valgrind
VGFLAG	= --leak-check=full -v
VGCFLAG = -g -O0

GDB	= gdb
DBFLAG	= 

.PHONY: default
default: $(PROG)

.PHONY: all
all: clean $(PROG)

bin/%.o: src/%.c $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $<

.PHONY: val_set
val_set:
	$(eval CFLAGS += $(VGCFLAG))

.PHONY: val_build
val_build: clean val_set $(PROG)

.PHONY: val
val:	val_build
	$(VGRIND) $(VGFLAG) ./$(PROG)

.PHONY: gdb
gdb:	val_build
	$(GDB) $(DBFLAG) ./$(PROG)

.PHONY: $(PROG)
$(PROG): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

.PHONY: clean
clean:
	$(RM) $(RMFLAGS) $(PROG) $(OBJ) vgcore.*
