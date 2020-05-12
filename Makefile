include setup.mak

.PHONY: examples

OFILES = mallocWatch.o context.o cont.o store.o trail.o \
	trailable.o domain.o intvar.o solver.o \
	matching.o acstr.o constraint.o search.o controller.o \
    bitset.o literal.o explainer.o


all: $(LIBNAME) examples

test:
	make -C examples

$(LIBNAME): $(OFILES)
	$(CC) $(CXXFLAGS) $(OFILES) --shared $(LLIBFLAGS) -o $(LIBNAME)
	@if [ ! -f $(basename $(LIBNAME)) ];  \
	then \
	  ln -s $(LIBNAME) $(basename $(LIBNAME)); \
	fi

%.o : %.cpp %.hpp
	@echo "Compiling (C++)... " $<
	$(CC) -c $(CXXFLAGS) $<

%.d: %.cpp
	@set -e; rm -f $@; \
	$(CC) -M $(CXXFLAGS) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

clean:
	rm -rf $(OFILES) cpptest *~ *.d *.o $(LIBNAME)
	make -C examples clean

# This imports the dependency header specs.

include $(OFILES:.o=.d) 
