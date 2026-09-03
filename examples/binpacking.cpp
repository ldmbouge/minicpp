#include <sys/resource.h>

#include <iostream>
#include <fstream>
#include <numeric>

#include "cxxopts/cxxopts.hpp"

#include <libminicpp/solver.hpp>
#include <libminicpp/search.hpp>
#include <libminicpp/constraint.hpp>
#include <libfca/Timer.hpp>
#include <libfca/Utils.hpp>

#include <global_constraints/lex_chain_bit.hpp>
#include <global_constraints/bin_packing.hpp>
#include <gpu_constriants/bin_packing.cuh>

int main(int argc, char * argv[])
{
    // Parse CLI options
    cxxopts::Options options_parser("bpp-minicpp", "A GPU-accelerated Bin Packing solver based on MiniCPP.");
    options_parser.custom_help("[Options]");
    options_parser.positional_help("<BPPLIB file>");
    options_parser.add_options()
            ("s,stats", "Print search statistics", cxxopts::value<bool>())
            ("t", "Timeout in seconds", cxxopts::value<int>()->default_value("2147483647"))
            ("g,gpu", "Use GPU-accelerated lower bound", cxxopts::value<bool>())
            ("bpf", "BPPLIB file", cxxopts::value<std::string>())
            ("h,help", "Print usage");
    options_parser.parse_positional({"bpf"});
    auto args = options_parser.parse(argc, argv);

    if (args.count("bpf") == 1)
    {
        using namespace Fca::Utils::Math;

        // Increase stack size to 256 MB
        struct rlimit limit = {256 * 1024 * 1024, 256 * 1024 * 1024};
        setrlimit(RLIMIT_STACK, &limit);

        // Parameter
        int timeout = args["t"].as<int>();
        bool useGpu = args["gpu"].count() != 0;

        // Read BPPLIB file in BPP format
        int n_items;
        int capacity;
        std::vector<int> weights;
        std::string bpp_filename = args["bpf"].as<std::string>();
        std::ifstream bpp_file(bpp_filename);
        if (bpp_file.fail())
        {
            std::cerr << "File does not exists: " << bpp_filename << std::endl;
            return EXIT_FAILURE;
        }
        bpp_file >> n_items;
        bpp_file >> capacity;
        for (int wIdx = 0; wIdx < n_items; wIdx += 1)
        {
            int weight;
            bpp_file >> weight;
            weights.push_back(weight);
        }

        // Print BPPLIB instance
        std::cout << "Timeout: " << timeout << " s" << std::endl;
        std::cout << "Lowerbound: " << (useGpu ? "GPU" : "CPU") << std::endl;
        std::cout << "File: " << bpp_filename << std::endl;
        std::cout << "---" << std::endl;
        std::cout << "Capacity: " << capacity << std::endl;
        std::cout << "Items (" << n_items << "): ";
        for (int wIdx = 0; wIdx < n_items; wIdx += 1)
        {
            std::cout << weights.at(wIdx) << " ";
        }
        std::cout << std::endl;

        // Create Statistics
        SearchStatistics stats;
        stats.setStartTime();

        // Create Solver
        CPSolver::Ptr solver = Factory::makeSolver();

        // Calculate bounds to the number of bins
        int const sumWeights = std::accumulate(weights.begin(), weights.end(), 0);
        int const lb = ceilDivPosInt(sumWeights, capacity);
        int const ub = lb * 2;

        // Create variables
        var<int>::Ptr currNumBins = Factory::makeIntVar(solver, lb - 1, ub);
        std::vector<var<int>::Ptr> itemsBin;
        for (int iIdx = 0; iIdx < n_items; iIdx += 1)
        {
            itemsBin.push_back(Factory::makeIntVar(solver, 0, ub));
        }
        std::vector<var<int>::Ptr> binsLoad;
        for (int bIdx = 0; bIdx <= ub; bIdx += 1)
        {
            binsLoad.push_back(Factory::makeIntVar(solver, 0, capacity));
        }

        // Create constraints
        for (int iIdx = 0; iIdx < n_items; iIdx += 1)
        {
            solver->post(new(solver) LessOrEqual(itemsBin.at(iIdx), currNumBins));
        }
        solver->post(new (solver) LexChainBit(itemsBin));
        if (useGpu)
        {
            solver->post(new(solver) BinPackingLoadGPU(binsLoad, itemsBin, weights));
        }
        else
        {
            solver->post(new(solver) BinPackingLoad(binsLoad, itemsBin, weights));
        }


        // Search heuristic
        // Preallocate working memory to avoid leaks
        struct BinInfo
        {
            int index;
            int residual;
        };
        std::vector<int> residualCapacity(n_items, capacity);
        std::vector<BinInfo> binsResidualDecreasing(n_items);
        std::vector<int> candidates(n_items, 0);
        auto complete_decreasing_best_fit = [&](CPSolver::Ptr const & cp, std::vector<var<int>::Ptr> & itemsBin)
        {
            using namespace Factory;

            // Calculate the residual capacity of every bin
            int const nBins = currNumBins->max() + 1;
            std::fill(residualCapacity.begin(), residualCapacity.end(), capacity);
            for (int iIdx = 0; iIdx < n_items; iIdx += 1)
            {
                if (itemsBin.at(iIdx)->isBound())
                {
                    int bIdx = itemsBin.at(iIdx)->min();
                    int iWeight = weights.at(iIdx);
                    residualCapacity.at(bIdx) -= iWeight;
                }
            }

            // Dominance rules
            bool someItemAssigned = true;
            while (someItemAssigned)
            {
                someItemAssigned = false;

                // Perfect fit
                for (int bIdx = 0; bIdx < nBins; bIdx += 1)
                {
                    auto & bResidualCapacity = residualCapacity.at(bIdx);
                    for (int iIdx = 0; iIdx < n_items; iIdx += 1)
                    {
                        int iWeight = weights.at(iIdx);
                        auto & iBins = itemsBin.at(iIdx);
                        if (not iBins->isBound() and iBins->contains(bIdx) and bResidualCapacity - iWeight == 0)
                        {
                            bResidualCapacity = 0;
                            iBins->assign(bIdx);
                            someItemAssigned = true;
                            break;
                        }
                    }
                }

                // Only one item fits
                binsResidualDecreasing.clear();
                for (int bIdx = 0; bIdx < nBins; bIdx += 1)
                {
                    auto & bResidualCapacity = residualCapacity.at(bIdx);
                    binsResidualDecreasing.push_back({bIdx, bResidualCapacity});
                }
                std::sort(binsResidualDecreasing.begin(), binsResidualDecreasing.end(), [](auto & lhs, auto & rhs)
                { return lhs.residual > rhs.residual; });

                for (int brIdx = 0; brIdx < nBins; brIdx += 1)
                {
                    int bIdx = binsResidualDecreasing.at(brIdx).index;
                    candidates.clear();
                    auto & bResidualCapacity = residualCapacity.at(bIdx);
                    for (int iIdx = 0; iIdx < n_items; iIdx += 1)
                    {
                        int iWeight = weights.at(iIdx);
                        auto & iBin = itemsBin.at(iIdx);
                        if (not iBin->isBound() and iBin->contains(bIdx) and bResidualCapacity >= iWeight)
                        {
                            candidates.push_back(iIdx);
                        }
                    }
                    int nCandidates = (int) candidates.size();
                    if (nCandidates > 1)
                    {
                        int smallestCandidateWeight1 = weights.at(candidates.at(nCandidates - 1));
                        int smallestCandidateWeight2 = weights.at(candidates.at(nCandidates - 2));
                        if (smallestCandidateWeight1 + smallestCandidateWeight2 > bResidualCapacity)
                        {
                            int biggerCandidateIdx = candidates.at(0);
                            int biggerCandidateWeight = weights.at(biggerCandidateIdx);
                            bResidualCapacity -= biggerCandidateWeight;
                            itemsBin.at(biggerCandidateIdx)->assign(bIdx);
                            someItemAssigned = true;
                        }
                    }
                }
            }

            cp->fixpoint();

            // Select biggest item
            auto itemBin = selectMax(itemsBin,
                                     [&](const auto & x)
                                     {
                                         return x->size() > 1;
                                     },
                                     [&](const auto & x)
                                     {
                                         auto it = std::find(itemsBin.begin(), itemsBin.end(), x);
                                         int const iIdx = std::distance(itemsBin.begin(), it);
                                         int const weight = weights.at(iIdx);
                                         return weight;
                                     });

            Branches branches({});

            // Select best fit bin
            if (itemBin)
            {
                // Calculate item weight
                auto it = std::find(itemsBin.begin(), itemsBin.end(), itemBin);
                int iIdx = static_cast<int>(std::distance(itemsBin.begin(), it));
                int iWeight = weights.at(iIdx);

                // Calculate best bin
                int bestBin = INT_MIN;
                int bestResidualCapacity = INT_MAX;
                for (int bIdx = 0; bIdx < nBins; bIdx += 1)
                {
                    if (itemBin->contains(bIdx))
                    {
                        int candidateResidualCapacity = residualCapacity.at(bIdx) - iWeight;
                        if (0 <= candidateResidualCapacity and candidateResidualCapacity < bestResidualCapacity)
                        {
                            bestBin = bIdx;
                            bestResidualCapacity = candidateResidualCapacity;
                        }
                    }
                }

                branches.add([=]
                             {
                                 // Best bin assignment
                                 cp->post(itemBin == bestBin);
                             });
                branches.add([=]
                             {
                                 int nBins = currNumBins->max() + 1;
                                 int targetResidualCapacity = residualCapacity.at(bestBin);

                                 // Removing all equivalent bins
                                 for (int bIdx = 0; bIdx < nBins; bIdx += 1)
                                 {
                                     if (residualCapacity.at(bIdx) == targetResidualCapacity)
                                     {
                                         // For all equivalent items
                                         for (int jIdx = iIdx; jIdx < n_items and iWeight == weights.at(jIdx); jIdx += 1)
                                         {
                                             if (not itemsBin.at(jIdx)->isBound())
                                             {
                                                 itemsBin.at(jIdx)->remove(bIdx);
                                             }
                                         }
                                     }
                                 }
                                 cp->fixpoint();
                             });

            }
            return branches;
        };

        // Create Search
        auto bound_search = [&](){ return indomain_min(solver, input_order<std::vector<var<int>::Ptr>, var<int>::Ptr>({currNumBins})); };
        auto item_search = [&](){ return complete_decreasing_best_fit(solver, itemsBin); };
        DFSearch search(solver, land<std::function<Branches(void)>>({bound_search, item_search}));

        // Collect statistics
        search.onSolution([&](){stats.incrSolutions();});
        search.onBranch([&](){ stats.incrNodes();});
        search.onFailure([&](){stats.incrFailures();});

        // Search limits
        Limit search_limits = [&](SearchStatistics const & search_statistics)
        {
            return search_statistics.getRunningTime() >= timeout or
                   search_statistics.getSolutions() > 0;
        };
        // Output
        search.onSolution(
                [&]()
                {
                    //assert(currNumBins->isBound());
                    std::cout << "Objective: " << currNumBins->max() + 1 << std::endl;
                    std::cout << "Packing: ";
                    for (int iIdx = 0; iIdx < n_items; iIdx += 1)
                    {
                        assert(itemsBin.at(iIdx)->isBound());
                        std::cout << itemsBin.at(iIdx)->min() << " ";
                    }
                    std::cout << std::endl;
                    std::cout << "Loads: ";
                    std::vector<int> load(currNumBins->max() + 1, 0);
                    for (int iIdx = 0; iIdx < n_items; iIdx += 1)
                    {
                        load.at(itemsBin.at(iIdx)->min()) += weights.at(iIdx);
                    }
                    for (auto const & l: load)
                    {
                        assert(0 < l);
                        assert(l <= capacity);
                        std::cout << l << " ";
                    }
                    std::cout << std::endl;
                });

        // Launch Search
        stats.setSearchStartTime();
        search.solve(stats, search_limits);
        stats.setSearchEndTime();

        // Print statistics
        if (args["s"].count() != 0)
        {
            std::cout << std::fixed << std::setprecision(3)
                      << "---" << std::endl
                      << "Time: " << stats.getSolveTime() << " s" << std::endl
                      << "Propagations: " << solver->getPropagations() << std::endl
                      << "Nodes: " << 1 + stats.getNodes() + stats.getTighteningFail() << std::endl
                      << "Failures: " << stats.getFailures()  - stats.getTighteningFail() << std::endl
                      << "Peak Depth: " << search.getPeakDepth() << std::endl;
        }
    }
    else
    {
        std::cout << options_parser.help();
        exit(EXIT_FAILURE);
    }
}
