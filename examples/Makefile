include ../setup.mak

CXXFLAGS += -I..

LFLAGS = -L$(shell pwd)/.. 

BENCHSRCS = mainCont.cpp main.cpp main2.cpp main3.cpp nqperf.cpp magicserie.cpp qap.cpp
BENCHS = test0 test1 test2 test3 nqperf magicserie qap

all: cpptests 

cpptests: $(BENCHS)

run: test1
	test1

test1: main.o
	$(CC) $(CXXFLAGS) $< -l$(LIBBASE) $(LFLAGS) -o $@

test2: main2.o
	$(CC) $(CXXFLAGS) $< -l$(LIBBASE) $(LFLAGS) -o $@

test3: main3.o
	$(CC) $(CXXFLAGS) $< -l$(LIBBASE) $(LFLAGS) -o $@

nqperf: nqperf.o
	$(CC) $(CXXFLAGS) $< -l$(LIBBASE) $(LFLAGS) -o $@

magicserie: magicserie.o
	$(CC) $(CXXFLAGS) $< -l$(LIBBASE) $(LFLAGS) -o $@

qap: qap.o
	$(CC) $(CXXFLAGS) $< -l$(LIBBASE) $(LFLAGS) -o $@

test0: mainCont.o
	$(CC) $(CXXFLAGS) $< -l$(LIBBASE) $(LFLAGS)  -o $@

%.o : %.cpp
	@echo "Compiling (C++)... " $<
	@$(CC) -c $(CXXFLAGS) $<

%.d: %.cpp
	@set -e; rm -f $@; \
	$(CC) -M $(CXXFLAGS) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

clean:
	rm -rf *.o *.d $(BENCHS)

include $(BENCHSRCS:.cpp=.d) 
