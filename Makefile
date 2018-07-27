HOST=$(shell uname)

CXXFLAGS=-g -std=c++14
#CXXFLAGS=-O3 -std=c++14

ifeq ($(HOST),Darwin)
CXXFLAGS += -fPIC
CC=c++
LLIBFLAGS=
LFLAGS= -L.
else
CXXFLAGS += -fPIC
CC=clang++-4.0
LLIBFLAGS=-Wl,-soname,$(LIBNAME)
LFLAGS  = -L. -Wl,-rpath=`pwd`
endif

OFILES = mallocWatch.o context.o cont.o store.o trail.o \
	engine.o reversible.o BitDomain.o intvar.o solver.o constraint.o search.o controller.o

LIBBASE = copl
LIBNAME = lib$(LIBBASE).so.1

all:   $(LIBNAME) cpptests

$(LIBNAME): $(OFILES)
	$(CC) $(CXXFLAGS) $(OFILES) --shared $(LLIBFLAGS) -o $(LIBNAME)
	@if [ ! -f $(basename $(LIBNAME)) ];  \
	then \
	  ln -s $(LIBNAME) $(basename $(LIBNAME)); \
	fi

cpptests: test1 test2

test1: main.o
	$(CC) $(CXXFLAGS) $< -l$(LIBBASE) $(LFLAGS) -o $@

test2: main2.o
	$(CC) $(CXXFLAGS) $< -l$(LIBBASE) $(LFLAGS) -o $@

test3: mainCont.o
	$(CC) $(CXXFLAGS) $< -l$(LIBBASE) $(LFLAGS)  -o $@

run: test1
	test1

%.o : %.cpp
	$(CC) -c $(CXXFLAGS) $<

%.d: %.cpp
	@set -e; rm -f $@; \
	$(CC) -M $(CXXFLAGS) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

clean:
	rm -rf $(OFILES) cpptest *~ *.d

# This imports the dependency header specs.

include $(OFILES:.o=.d) main.d main2.d
