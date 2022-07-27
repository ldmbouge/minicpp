# CP 2022 Tutorial: MDD-Based Constraint Programming in Haddock

# The VM

We provide a VirtualBox VM with Ubuntu 22.04 LTS (Server) edition *for x86_64*. The VM 
should have everything you need (Provided you have VirtualBox 6.1 installed). 
We installed both VIM and Emacs. The VM is a console only VM (to keep it  small). 

The MiniCPP code is already cloned, configured (with `cmake`) and 
compiled. You only have to bring up the VM (which is NAT'd, not bridged). So
if you opt to use it, you can skip to the last section that covers where 
the files are!

## Credentials

The credentials for the VM are as follows

	user: cp
	password: cp2022

## VM details

- Architecture: x86_64
- Ubuntu 22.04 LTS Server Edition
- NAT'd ethernet
- 1 Core
- 4G of RAM
- 10G virtual disk
- 5.15G for VirtualBox image
- No UI (login via the console)

# Supported OS

Minicpp is developed on macOS and compiles and runs routinely on Linux.
It supports both x86_64 and ARM64 (M1/M2). It may just work on windows, though 
I do not test that (you'd need llvm/clang to compile it of course)

# Getting MiniCPP on your machine (if you wish!)

The provided VMs already have the source code checked out in a folder 
`$HOME/work/minicpp`. If you are obtaining the code on your machine directly,
then, all you need to do is:

```
git clone https://ldmbouge@bitbucket.org/ldmbouge/minicpp.git
```

And the library will get cloned into your current working directory in a 
folder named `minicpp`

# Compiling
	
You need a C++ compiler supporting C++-17 as well as `cmake`. To compile,
simply create a `build` folder, move into that folder, create the build
system with the `cmake` command and then use `make` as usual. 

TLDR (assuming you have minicpp checked out in the folder `~/work/minicpp`)

```
cd ~/work/minicpp
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j4
```

The `-j4` is to compile in parallel. Feel free to increase to whatever your
machine can take!

For the tutorial, the CMakeLists.txt file has all the other examples commented
out and it will only compile the examples we will use. If you wish to restore 
all the examples, you can edit that file and uncomment like 94 (and comment line 95)

```
#file(GLOB files "examples/*.cpp" "examples/mdd/*.cpp" "examples/tutorial/*.cpp")
file(GLOB files "examples/tutorial/*.cpp")
```

Of course, you need to rerun `cmake` if you change this file. If you wish to 
debug code, you should just run `cmake` as follows

```
cmake ..
```

Namely, the default is to compile in Debug! If you need a debugger, `gdb` 
and `lldb` are both installed.

# The tutorial

The examples for the tutorial are located in 

```
~/work/minicpp/examples/tutorial
```

Some are covered in the slides, some are handout for you to work with 
(the names end in `Handout.cpp` in that case) and some are the "solutions" or
classic models for reference sake.

The compiled files will appear in 

```
~/work/minicpp/build
```

# Documentation

There is *preliminary* documentation (produced by `doxygen`) in 

```
~/work/minicpp/doc/html
```

Just open `index.html` in your favorite browser and go from there. If you wish 
to generate the documentation, run

```
cd ~/work/minicpp
doxygen minicpp
```

And the doc will get refreshed.



