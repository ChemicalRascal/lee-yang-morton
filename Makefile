CC  	= g++
CFLAGS	= -Wall
LFLAGS	= -lsdsl -ldivsufsort -ldivsufsort64
RM	= rm
RMFLAGS	= -f
MKDIR	= mkdir
MDFLAGS	= -p

AVX2FLAG= -mavx2

PROG	= qtree_testbench
DEPS	= src/bitseq.hpp src/qsi.hpp src/leeyang.hpp src/read_csv.h \
	  src/morton.h
OBJ	= bin/bitseq.obj bin/qsi.obj bin/leeyang.obj bin/read_csv.o \
	  bin/morton.o bin/qtree_testbench.obj
CFILES	= src/bitseq.cpp src/qsi.cpp src/leeyang.cpp src/read_csv.c \
	  src/morton.c src/qtree_testbench.cpp
BINDIR	= bin/

PROG2	= gen_queries
CFILES2	= src/gen_queries.c

PROG3	= test_ostream
CFILES3	= src/test_ostream.cpp

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
	$(CC) $(CFLAGS) -c $< -o $@

bin/%.obj: src/%.cpp $(DEPS) $(BINDIR)
	$(CC) $(CFLAGS) -c $< -o $@

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
	$(CC) $(CFLAGS) $^ -o $@ $(LFLAGS)

$(PROG2): $(CFILES2)
	$(CC) $(CFLAGS) $(CFILES2) -o $@

$(PROG3): $(CFILES3) $(DEPS)
	$(CC) $(CFLAGS) $(CFILES3) bin/bitseq.obj -o $@ $(LFLAGS)

.PHONY: clean
clean:
	$(RM) $(RMFLAGS) $(PROG) $(OBJ) vgcore.* $(PROG2) $(PROG3)
