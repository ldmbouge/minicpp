// Bin Packing with the lower bounds of the bin packing constraint computed on the GPU.
// Same model and search as basic/binpacking.cpp (see basic/binpacking.hpp); -a CPU runs the CPU
// constraint instead, to compare.
// Reference: CP for Bin Packing with Multi-Core and GPUs (DOI: 10.4230/LIPIcs.CP.2024.28)

#include <CLI/CLI.hpp>
#include "../basic/binpacking.hpp"
#include "constraints_gpu/binpacking.cuh"

int main(int argc, char* argv[])
{
   std::string instance = "data/bpp/Scholl2/N3W2B1R1.BPP";
   int timeout = INT_MAX;
   bool printStats = false;
   std::string lowerbound = "GPU";

   CLI::App app{"A GPU-accelerated MiniCPP-based Bin Packing solver"};
   app.add_option("-i,instance", instance, "BPPLIB instance file (.BPP)")->check(CLI::ExistingFile);
   app.add_option("-t", timeout, "Timeout in seconds");
   app.add_flag("-s,--stats", printStats, "Print search statistics");
   app.add_option("-a", lowerbound, "Where the lower bounds are computed")
      ->check(CLI::IsMember({"CPU", "GPU"}));
   CLI11_PARSE(app, argc, argv);

   bool const useGpu = lowerbound == "GPU";
   return solveBinPacking(instance, timeout, printStats, lowerbound,
      [useGpu](auto cp, auto const& load, auto const& bin, auto const& w) -> Constraint* {
         if (useGpu)
            return new (cp) BinPackingLoadGPU(load, bin, w);
         return new (cp) BinPackingLoad(load, bin, w);
      });
}
