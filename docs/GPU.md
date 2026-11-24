# To run the GPU Demo

First, configure cmake with the GPU turned on.
Make sure the nvidia libraries and tools are available. cmake should find them

```
cmake -DCMAKE_BUILD_TYPE=Release -DENABLE_GPU=ON
```

Then compile

```
make -j10
```

And finally run rcpsp

```
./rcpsp ../data/rcpsp/J60_10_1.json
```

To see the effect (or lack thereof of the GPU, go in the model and edit the propagator creation to use the CPU
version rather than the GPU version.

That is:

```
emacs ../examples/gpu/rcpsp.cu
```
The extension (`cu`)  tells to invoke the C++ version that supports CUDA (line 68). That's a CPP file really. The matching
header  (if one existed) would be  in `../examples/gpu/rcpsp.cuh`. Note that this is the case for the propagator
source code (the `*_gpu.cu` and `*_gpu.cuh` files in `../src/constraints/`).

On a test machine @UConn, the difference is:

| Bench    | CPU  | GPU  |
|----------|------|------|
| j60_10_1 | 2.6s | 0.05 |
|          |      |      |

# Bin Packing

`binpacking_gpu` solves Bin Packing instances (BPPLIB format, in `data/bpp/`) with the lower bounds of the
bin packing constraint computed on the GPU (`src/constraints/binpacking_gpu.cu`). `-a CPU` switches to the CPU
constraint with the same model and search, and `-s` prints the search statistics:

```
./binpacking_gpu -s -t 10 -i data/bpp/Scholl2/N3W2B1R1.BPP
./binpacking_gpu -s -t 10 -a CPU -i data/bpp/Scholl2/N3W2B1R1.BPP
```

The kernels use `__reduce_max_sync`, which needs a GPU of compute capability 8.0 (Ampere) or newer.
The CPU-only build has the same model as `binpacking` (`examples/basic/binpacking.cpp`).
