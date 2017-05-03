CC  	= gcc
CFLAGS	= -Wall
RM	= rm
RMFLAGS	= -f
MKDIR	= mkdir
MDFLAGS	= -p

AVX2FLAG= -mavx2

PROG	= qtree_testbench
DEPS	= src/bitseq.h src/qsi.h src/leeyang.h src/read_csv.h \
	  src/morton.h
OBJ	= bin/bitseq.o bin/qsi.o bin/leeyang.o bin/read_csv.o \
	  bin/morton.o bin/qtree_testbench.o
CFILES	= src/bitseq.c src/qsi.c src/leeyang.c src/read_csv.c \
	  src/morton.c src/qtree_testbench.c
BINDIR	= bin/

PROG2	= gen_queries
CFILES2	= src/gen_queries.c

VGRIND	= valgrind
VGFLAG	= --leak-check=full -v
VGCFLAG = -g -O0

GDB	= gdb
DBFLAG	= 

.PHONY: default
default: $(PROG) $(PROG2)

.PHONY: all
all: clean $(PROG) $(PROG2)

.PHONY: avx2
avx2: clean avx2_set all

$(BINDIR):
	$(MKDIR) $(MDFLAGS) $(BINDIR)

bin/%.o: src/%.c $(DEPS) $(BINDIR)
	$(CC) $(CFLAGS) -c -o $@ $<

.PHONY: val_set
val_set:
	$(eval CFLAGS += $(VGCFLAG))

.PHONY: avx2_set
avx2_set:
	$(eval CFLAGS += $(AVX2FLAG))

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

.PHONY: $(PROG2)
$(PROG2): $(CFILES2)
	$(CC) $(CFLAGS) -o $@ $(CFILES2)

.PHONY: clean
clean:
	$(RM) $(RMFLAGS) $(PROG) $(OBJ) vgcore.* $(PROG2)
