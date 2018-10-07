HOST=$(shell uname)

#CXXFLAGS=-O0 -g3 -std=c++14
CXXFLAGS=-O3 -std=c++14

LIBBASE = copl

ifeq ($(HOST),Darwin)

WARN = -Wnon-modular-include-in-framework-module -Werror=non-modular-include-in-framework-module -Wno-trigraphs \
	-Wno-missing-field-initializers -Wno-missing-prototypes -Werror=return-type -Wdocumentation -Wunreachable-code \
	-Werror=deprecated-objc-isa-usage -Werror=objc-root-class -Wno-non-virtual-dtor -Wno-overloaded-virtual -Wno-exit-time-destructors \
	-Wno-missing-braces -Wparentheses -Wswitch -Wunused-function -Wno-unused-label -Wno-unused-parameter -Wunused-variable -Wunused-value \
	-Wempty-body -Wuninitialized -Wconditional-uninitialized -Wno-unknown-pragmas -Wno-shadow -Wno-four-char-constants \
	-Wno-conversion -Wconstant-conversion -Wint-conversion -Wbool-conversion -Wenum-conversion -Wno-float-conversion \
	-Wnon-literal-null-conversion -Wobjc-literal-conversion -Wshorten-64-to-32 -Wno-newline-eof -Wno-c++11-extensions \
	-Wno-sign-conversion -Winfinite-recursion -Wmove -Wcomma -Wblock-capture-autoreleasing -Wstrict-prototypes \
	-Wrange-loop-analysis -Wno-semicolon-before-method-body -Wunguarded-availability \
	-fstandalone-debug -fno-limit-debug-info 
#	-fno-omit-frame-pointer -fno-optimize-sibling-calls

CWD=$(shell pwd)
CXXFLAGS += $(WARN) -fPIC
CC=c++
LLIBFLAGS=  -install_name '$(CWD)/$(LIBNAME)' -current_version 1.0
LIBNAME = lib$(LIBBASE).dylib
LFLAGS= -L. -dynamiclib -undefined suppress -flat_namespace
LFLAGSX=
else

CWD=$(shell pwd)
CXXFLAGS += -fPIC
CC=clang++-6.0
LLIBFLAGS=-Wl,-soname,$(LIBNAME)
LFLAGSX  = -L. -Wl,-rpath=$(CWD)/..
LIBNAME = lib$(LIBBASE).so.1



endif

