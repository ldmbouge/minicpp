include ../setup.mak

CXXFLAGS += -I..

LFLAGS = -L$(shell pwd)/.. $(LFLAGSX)

# mainCont.cpp

BENCHSRCS = main.cpp main2.cpp main3.cpp nqperf.cpp \
	magicserie.cpp qap.cpp tsp.cpp  magicsquare.cpp msperf.cpp \
	stablemarriage.cpp steel.cpp qaplns.cpp nqueensAC.cpp adac.cpp \
	mddamong.cpp mdd/among2.cpp mdd/among4.cpp mdd/amongsum.cpp mdd/tinymdd.cpp \
	mdd/tinyAllDiff.cpp

BENCHS =

# test0 

AT = main main2 main3 adac nqperf magicserie qap tsp magicsquare \
	msperf stablemarriage  steel qaplns nqueensAC mddamong \
	mdd/among2 mdd/among4 mdd/amongsum mdd/tinymdd mdd/tinyAllDiff

all: cpptests 

cpptests: $(BENCHS) $(AT)

run: main
	main

test0: mainCont.o
	@echo "Linking... " $@
	$(CC) $(CXXFLAGS) $< -l$(LIBBASE) $(LFLAGS)  -o $@

% : %.cpp 
	@echo "Compile(C++) & Link... " $@
	$(CC) $(CXXFLAGS) $< -l$(LIBBASE) $(LFLAGS) -o $@

%.o : %.cpp
	@echo "Compiling (C++)... " $<
	@$(CC) -c $(CXXFLAGS) $<

%.d: %.cpp
	@set -e; rm -f $@; \
	$(CC) -M $(CXXFLAGS) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

clean:
	rm -rf *.o *.d *.dSYM $(BENCHS) $(AT)

include $(BENCHSRCS:.cpp=.d) 
