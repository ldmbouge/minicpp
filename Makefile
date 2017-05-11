#CXXFLAGS=-g -std=c++14
CXXFLAGS=-O3 -std=c++14
CC=c++
OFILES = mallocWatch.o context.o cont.o \
	engine.o reversible.o BitDomain.o intvar.o solver.o constraint.o search.o 

LIBBASE = copl
LIBNAME = lib$(LIBBASE).so.1

all:   $(LIBNAME) cpptests

$(LIBNAME): $(OFILES)
	$(CC) $(CXXFLAGS) $(OFILES) --shared -o $(LIBNAME)
	@if [ ! -f $(basename $(LIBNAME)) ];  \
	then \
	  ln -s $(LIBNAME) $(basename $(LIBNAME)); \
	fi

cpptests: test1 test2

test1: main.o
	$(CC) $(CXXFLAGS) $< -L. -l$(LIBBASE) -o $@ 

test2: mainCont.o
	$(CC) $(CXXFLAGS) $< -L. -l$(LIBBASE) -o $@

%.o : %.cpp
	$(CC) -c $(CXXFLAGS) $<

%.d: %.cpp
	@set -e; rm -f $@; \
	$(CC) -M $(CFLAGS) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

clean:
	rm -rf $(OFILES) cpptest *~ *.d

# This imports the dependency header specs.

include $(OFILES:.o=.d)

