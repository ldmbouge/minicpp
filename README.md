# MiniCPP
A *C++* CP Solver that implements the
[MiniCP](https://bitbucket.org/minicp/minicp/src/master/) API.  The
architecture is identical to [that of MiniCP](http://minicp.org/) and
can be found in the paper [MiniCP: A lightweight solver for constraint
programming](https://link.springer.com/article/10.1007/s12532-020-00190-7)
published in [Mathematical Programming
Computation](https://www.springer.com/journal/12532) and best cited as

	@article{cite-key,
        Author = {Michel, L. and Schaus, P. and Van Hentenryck, P.},
        Doi = {10.1007/s12532-020-00190-7},
        Id = {Michel2021},
        Isbn = {1867-2957},
        Journal = {Mathematical Programming Computation},
        Number = {1},
        Pages = {133-184},
        Title = {MiniCP: a lightweight solver for constraint programming},
        Ty = {JOUR},
        Url = {https://doi.org/10.1007/s12532-020-00190-7},
        Volume = {13},
        Year = {2021}}
 
# MDD Library

MiniCPP includes [Haddock](https://dlnext.acm.org/doi/10.1007/978-3-030-58475-7_31) which is a library for defining
Multi-valued Decision Diagrams. 

	@inproceedings{DBLP:conf/cp/GentzelMH20,
	author    = {Rebecca Gentzel and Laurent Michel and Willem Jan van Hoeve},
	editor    = {Helmut Simonis},
	title     = {{HADDOCK:} {A} Language and Architecture for Decision Diagram Compilation},
	booktitle = {Principles and Practice of Constraint Programming - 26th International
               Conference, {CP} 2020, Louvain-la-Neuve, Belgium, September 7-11,
               2020, Proceedings},
	series    = {Lecture Notes in Computer Science},
	volume    = {12333},
	pages     = {531--547},
	publisher = {Springer},
	year      = {2020},
	url       = {https://doi.org/10.1007/978-3-030-58475-7\_31},
	doi       = {10.1007/978-3-030-58475-7\_31},
	timestamp = {Sat, 09 Apr 2022 12:47:09 +0200},
	biburl    = {https://dblp.org/rec/conf/cp/GentzelMH20.bib},
	bibsource = {dblp computer science bibliography, https://dblp.org}
		}

# Bin Packing

MiniCPP includes the Bin Packing solver of [CP for Bin Packing with Multi-Core and GPUs](https://doi.org/10.4230/LIPIcs.CP.2024.28):
the `BinPackingLoad` constraint (`src/constraints/binpacking.hpp`), with lower bounds from Dual-Feasible Functions
that can also be computed on the GPU (`src/constraints/binpacking_gpu.cuh`). Instances from
[BPPLIB](https://site.unibo.it/operations-research/en/research/bpplib-a-bin-packing-problem-library)
are in `data/bpp/`. The example models are `examples/basic/binpacking.cpp` (CPU) and
`examples/gpu/binpacking_gpu.cu` (GPU), sharing the model and search in `examples/basic/binpacking.hpp`.
From the `build/examples` folder:
```
./binpacking -s -t 10 -i data/bpp/FalkenauerT/Falkenauer_t120_00.BPP
./binpacking_gpu -s -t 10 -i data/bpp/Scholl2/N3W2B1R1.BPP   # needs -DENABLE_GPU=ON
```
`-t` is the timeout in seconds and `-s` prints the search statistics; `binpacking_gpu -a CPU` runs the
CPU constraint for comparison (see the [GPU build](docs/GPU.md)).

	@inproceedings{Tardivo2024,
	author    = {Fabio Tardivo and Laurent Michel and Enrico Pontelli},
	title     = {{CP} for Bin Packing with Multi-Core and {GPUs}},
	booktitle = {30th International Conference on Principles and Practice of Constraint Programming, {CP} 2024},
	series    = {LIPIcs},
	volume    = {307},
	publisher = {Schloss Dagstuhl -- Leibniz-Zentrum f{\"{u}}r Informatik},
	year      = {2024},
	doi       = {10.4230/LIPIcs.CP.2024.28}
		}

# Cumulative

MiniCPP includes the Cumulative constraint of [Constraint propagation on GPU: a case study for the cumulative
constraint](https://doi.org/10.1007/s10601-024-09371-w), with Time-Tabling (`cumulativeTT`, `src/constraints/ttable.hpp`)
and Energetic Reasoning (`cumulativeER`, `src/constraints/cumulative.hpp`) propagators; Energetic Reasoning can also
run on the GPU (`src/constraints/cumulative_gpu.cuh`). The factories are in `src/scheduling/scheduling.hpp`.
The `rcpsp_cli` example (`examples/gpu/rcpsp_cli.cu`) solves Resource-Constrained Project Scheduling instances from
[PSPLIB](https://www.om-db.wi.tum.de/psplib/) (in `data/rcpsp/`), choosing the propagation with `-a`
(`TT`, `ER`, `ER-GPU` or `TT+ER-GPU`). It needs `-DENABLE_GPU=ON`; from the `build/examples` folder:
```
./rcpsp_cli -t 10 -a TT+ER-GPU -i data/rcpsp/psplib/J60_10_1.json
```

	@article{Tardivo2024Cumulative,
	author    = {Fabio Tardivo and Agostino Dovier and Andrea Formisano and Laurent Michel and Enrico Pontelli},
	title     = {Constraint propagation on {GPU}: a case study for the cumulative constraint},
	journal   = {Constraints},
	volume    = {29},
	pages     = {192--214},
	year      = {2024},
	doi       = {10.1007/s10601-024-09371-w}
		}

# Requirements to get started

You will need a recent `g++` / `clang` compiler as well as `cmake`. 

## Basic Compilation

The library is build routinely on macOS 12.14
and one only needs to do the following to get the basics in place:
```
cd <root>
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j4
```
Where `root` is the folder where you have decompressed the library (or cloned the repository)
You ought to be rewarded with a complete build. Feel free to omit the build type, or use `Debug` as the value
of that environment variable to compile in Debug mode.

To build and run the unit tests (in `tests/`), configure with `-DBUILD_TESTING=ON`, then from the `build` folder:
```
ctest --output-on-failure
```
The examples are built with `-DBUILD_EXAMPLES=ON`.

More documentation is in `docs/`: the [tutorial](docs/TUTORIAL.md), the [GPU build](docs/GPU.md),
and the API reference, generated by configuring with `-DBUILD_DOCS=ON` (needs `doxygen`).
