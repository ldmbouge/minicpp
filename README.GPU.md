# To run the GPU Demo

First, configure cmake with the GPU turned on.
Make sure the nvidia libraries and tools are available. cmake should find them

```
cmake -DCMAKE_BUILD_TYPE=Release -DGPU=on
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
emacs ../examples_gpu/rcpsp.cu
```
The extension (`cu`)  tells to invoke the C++ version that supports CUDA (line 68). That's a CPP file really. The matching
header  (if one existed) would be  in `../examples_gpu/rcpsp.cuh`. Note that this is the case for the propagator
source code (`.cu` and `.cuh` extensions in `../gpu_constraints/`).

On a test machine @UConn, the difference is:

| Bench    | CPU  | GPU  |
|----------|------|------|
| j60_10_1 | 2.6s | 0.05 |
|          |      |      |
