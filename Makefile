include setup.mak

.PHONY: examples minicpp_wrap.cxx

OFILES = mallocWatch.o store.o trail.o \
	trailable.o domain.o intvar.o solver.o \
	matching.o acstr.o constraint.o search.o 

PYNAME= minicpp$(shell python3-config --extension-suffix)
# context.o cont.o controller.o

all: $(LIBNAME) examples py

test:
	make -C examples

py: $(PYNAME)
	@echo $(PYNAME)

$(LIBNAME): $(OFILES)
	$(CC) $(CXXFLAGS) $(OFILES) --shared $(LLIBFLAGS) -o $(LIBNAME)
	@if [ ! -f $(basename $(LIBNAME)) ];  \
	then \
	  ln -s $(LIBNAME) $(basename $(LIBNAME)); \
	fi

%.o : %.cpp
	@echo "Compiling (C++)... " $<
	@$(CC) -c $(CXXFLAGS) $<

%.d: %.cpp
	@set -e; rm -f $@; \
	$(CC) -M $(CXXFLAGS) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$


$(PYNAME): pybridge.cpp libcopl.dylib
	$(CC) $(CXXFLAGS) -dynamiclib -std=c++14 -I`python3-config --includes` \
	-I/usr/local/Cellar/pybind11/2.4.3/include -L. `python3-config --ldflags` \
	$< -lcopl -lc++ -o minicpp`python3-config --extension-suffix`

%.o : %.cxx
	@echo "Compiling (CXX)... " $<
	$(CC) -c $(CXXFLAGS) `python3-config --cflags` $<

clean:
	rm -rf $(OFILES) cpptest *~ *.d *.o $(LIBNAME) *.so *.dSYM
	make -C examples clean

# This imports the dependency header specs.

include $(OFILES:.o=.d) 
