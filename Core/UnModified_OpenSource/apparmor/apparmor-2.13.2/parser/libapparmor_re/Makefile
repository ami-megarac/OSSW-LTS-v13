# Profiling:
#EXTRA_CFLAGS = -pg

ifdef USE_SYSTEM
  # Using the system libapparmor
  INCLUDE_APPARMOR =
else
  INCLUDE_APPARMOR = -I../../libraries/libapparmor/include
endif

TARGET=libapparmor_re.a

CFLAGS ?= -g -Wall -O2 ${EXTRA_CFLAGS} -std=gnu++0x
CXXFLAGS := ${CFLAGS} ${INCLUDE_APPARMOR}

ARFLAGS=-rcs

BISON := bison

all : ${TARGET}

UNITTESTS = tst_parse

libapparmor_re.a: parse.o expr-tree.o hfa.o chfa.o aare_rules.o
	ar ${ARFLAGS} $@ $^

expr-tree.o: expr-tree.cc expr-tree.h

hfa.o: hfa.cc apparmor_re.h hfa.h ../immunix.h

aare_rules.o: aare_rules.cc aare_rules.h apparmor_re.h expr-tree.h hfa.h chfa.h parse.h ../immunix.h

chfa.o: chfa.cc chfa.h ../immunix.h

parse.o : parse.cc apparmor_re.h expr-tree.h

parse.cc : parse.y parse.h flex-tables.h ../immunix.h
	${BISON} -o $@ $<

clean:
	rm -f *.o parse.cc ${TARGET} *.gcda *.gcno
