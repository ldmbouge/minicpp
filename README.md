## MiniCPP
A *C++* CP Solver that implements the MiniCP API.


# Requirements
To compile *fzn-minicpp* is enough to have a recent version of a GCC and CMake:
- GCC >= 7
- CMake >= 3.5

To modify the FlatZinc parser is necessary to have a recent version of Flex and Bison:
- Flex >= 2.6
- Bison >= 3.8
      
To cross-compile and generate Windows binaries is necessary have a modern version og MinGW64: 
- MinGW-w64 >= 9.3


# Compile
1) Launch the configuration script, for example:
```
./autoCMake.sh -l -r
```
Please see `./autoCmake --help` for all the options.

2) Launch the make process, for example:
```
cd ./cmake-build-linux-release
make fzn-minicpp -j 4
```

# Configure MiniZinc IDE
Select from the toolbar: MiniZinc > Preferences > Solver > Add new. Then configure as follow:

<img src="./MiniZinc-cfg.png" width="200">


