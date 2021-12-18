#!/bin/bash

function print_help
{
    echo "Usage: ./autoCMake.sh <System type> <Build type> [Build directory]"
    echo "  System type:"
    echo "    -l  Linux"
    echo "    -w  Windows"
    echo "  Build type:"
    echo "    -d  Debug"
    echo "    -r  Release"
    echo "Example: ./autoCMake.sh -l -d"
}

#Input parsing
if [ "$#" -lt 2 ];
then
    print_help
    exit 1
fi
system_type=""
build_type=""
for arg in $1 $2
do
    case "$arg" in
        -l)
            system_type="Linux"
            ;;
        -w)
            system_type="Windows"
            ;;
        -r)
            build_type="Release"
            ;;
        -d)
            build_type="Debug"
            ;;
        -h|--help)
            printHelp
            exit 0
            ;;
        *)
            echo "[ERROR] Unrecognized argument: $arg"
            printHelp
            exit 1
            ;;
    esac
done

#Toolchain selection
c_compiler=""
cxx_compiler=""
if [ "$system_type" = "Linux" ];
  then
    c_compiler="gcc"
    cxx_compiler="g++"
elif [ "$system_type" = "Windows" ];
  then
    c_compiler="i686-w64-mingw32-gcc"
    cxx_compiler="i686-w64-mingw32-g++"
fi

#Build directory
build_dir=$3
if [ -z "$build_dir" ]
  then
    build_dir="cmake-build-${system_type,,}-${build_type,,}"
fi
rm -rf $build_dir &> /dev/null
mkdir -p build_dir

#CMake
cmake \
  -B $build_dir \
  -D CMAKE_SYSTEM_NAME=$system_type \
  -D CMAKE_C_COMPILER=$c_compiler \
  -D CMAKE_CXX_COMPILER=$cxx_compiler \
  -D CMAKE_BUILD_TYPE=$build_type \
  -D CMAKE_EXE_LINKER_FLAGS="-static-libgcc -static-libstdc++ -static"