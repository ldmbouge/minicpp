CXXFLAGS=-g -std=c++14
#CXXFLAGS=-O3 -std=c++14
CC=c++
OFILES = engine.o reversible.o BitDomain.o intvar.o solver.o constraint.o search.o main.o

all:   cpptest

cpptest: $(OFILES)
	$(CC) $(CXXFLAGS) -o cpptest $(OFILES)

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

