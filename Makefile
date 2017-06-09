CXXFLAGS=-g -std=c++14 -fPIC 
#CXXFLAGS=-O3 -std=c++14 -fPIC

CC=clang++-4.0
# #CC=c++

OFILES = mallocWatch.o context.o cont.o \
	engine.o reversible.o BitDomain.o intvar.o solver.o constraint.o search.o controller.o

LIBBASE = copl
LIBNAME = lib$(LIBBASE).so.1
LFLAGS  = -L. -Wl,-rpath=`pwd`

all:   $(LIBNAME) cpptests

$(LIBNAME): $(OFILES)
	$(CC) $(CXXFLAGS) $(OFILES) --shared -Wl,-soname,$(LIBNAME) -o $(LIBNAME) 
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

