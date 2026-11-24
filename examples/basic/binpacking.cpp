// Bin Packing: pack items of given weights into the fewest bins of a given capacity.
// The model and search are in binpacking.hpp; gpu/binpacking_gpu.cu is the GPU-accelerated version.
// Reference: CP for Bin Packing with Multi-Core and GPUs (DOI: 10.4230/LIPIcs.CP.2024.28)

#include <CLI/CLI.hpp>
#include "binpacking.hpp"

int main(int argc, char* argv[])
{
   std::string instance = "data/bpp/Scholl2/N3W2B1R1.BPP";
   int timeout = INT_MAX;
   bool printStats = false;

   CLI::App app{"A MiniCPP-based Bin Packing solver"};
   app.add_option("-i,instance", instance, "BPPLIB instance file (.BPP)")->check(CLI::ExistingFile);
   app.add_option("-t", timeout, "Timeout in seconds");
   app.add_flag("-s,--stats", printStats, "Print search statistics");
   CLI11_PARSE(app, argc, argv);

   return solveBinPacking(instance, timeout, printStats, "CPU",
      [](auto cp, auto const& load, auto const& bin, auto const& w) {
         return new (cp) BinPackingLoad(load, bin, w);
      });
}
