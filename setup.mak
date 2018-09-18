HOST=$(shell uname)

CXXFLAGS=-O0 -g -std=c++14
#CXXFLAGS=-O3 -std=c++14

LIBBASE = copl

ifeq ($(HOST),Darwin)

CWD=$(shell pwd)
CXXFLAGS += -fPIC
CC=c++
LLIBFLAGS=  -install_name '$(CWD)/$(LIBNAME)' -current_version 1.0
LIBNAME = lib$(LIBBASE).dylib
LFLAGS= -L. -dynamiclib -undefined suppress -flat_namespace

else

CXXFLAGS += -fPIC
CC=clang++-4.0
LLIBFLAGS=-Wl,-soname,$(LIBNAME)
LFLAGS  = -L. -Wl,-rpath=`pwd`
LIBNAME = lib$(LIBBASE).so.1



endif

