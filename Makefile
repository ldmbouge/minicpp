include setup.mak

.PHONY: examples minicpp_wrap.cxx

PYNAME= minicpp$(shell python3-config --extension-suffix)
# context.o cont.o controller.o

all: py

py: $(PYNAME)
	@echo $(PYNAME)

$(PYNAME): pybridge.cpp $(LIBNAME)
	$(CC) $< $(CXXFLAGS) -shared -std=c++14 -I`python3-config --includes` \
	`python3-config --ldflags` \
	`python3 -m pybind11 --includes` \
	-I/usr/local/Cellar/pybind11/2.4.3/include -L. -Wl,-rpath,`pwd` \
	-lcopl -o minicpp`python3-config --extension-suffix`

%.o : %.cxx
	@echo "Compiling (CXX)... " $<
	$(CC) -c $(CXXFLAGS) `python3-config --cflags` $<

clean:
	rm -rf $(OFILES) cpptest *~ *.d *.o $(LIBNAME) *.so *.dSYM
