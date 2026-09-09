#include "packing.hpp"
#include "utils.hpp"
#include "gfl/Backend.hpp"
#include <CLI/CLI.hpp>

#include <algorithm>
#include <fstream>
#include <numeric>

using namespace std;
using namespace Factory;
using namespace gfl;

// Read a BPPLIB instance in BPP format: number of items, bin capacity, one
// weight per line. Returns the bin capacity and the item weights.
pair<int, vector<int>> readInstance(string const & path)
{
    ifstream file(path);
    int nItems;
    int capacity;
    file >> nItems >> capacity;
    vector<int> weights(nItems);
    for (auto & weight : weights)
        file >> weight;
    sort(weights.begin(), weights.end(), std::greater<int>());
    return {capacity, weights};
}

// Complete Decreasing Best Fit: assign the heaviest unpacked item to the bin it
// fills best, after applying the dominance rules that leave no choice.
class DecreasingBestFit {
    CPSolver::Ptr _cp;
    Veci const & _bin;
    vector<int> const & _w;
    var<int>::Ptr & _nBins;
    int const _capacity;
    int const _nItems;
    vector<int> _residual;
    vector<int> _binsByResidual;

    bool fits(int i, int b) const { return !_bin[i]->isBound() && _bin[i]->contains(b) && _w[i] <= _residual[b]; }
    void calcResiduals() {
        fill(_residual.begin(), _residual.end(), _capacity);
        for (auto i = 0; i < _nItems; i += 1)
            if (_bin[i]->isBound())
                _residual[_bin[i]->min()] -= _w[i];
    }

    bool perfectFit(int nBins) {
        auto someItemAssigned = false;
        for (auto b = 0; b < nBins; b += 1)
            for (auto i = 0; i < _nItems; i += 1)
                if (fits(i, b) && _residual[b] == _w[i]) {
                    _residual[b] = 0;
                    _bin[i]->assign(b);
                    someItemAssigned = true;
                    break;
                }
        return someItemAssigned;
    }

    // When no two of the remaining items fit together in a bin, that bin can
    // take one more item at most, so it may as well take the heaviest one
    bool onlyOneItemFits(int nBins) {
        _binsByResidual.resize(nBins);
        iota(_binsByResidual.begin(), _binsByResidual.end(), 0);
        sort(_binsByResidual.begin(), _binsByResidual.end(), [&](auto lhs, auto rhs) { return _residual[lhs] > _residual[rhs]; });

        auto someItemAssigned = false;
        for (auto b : _binsByResidual) {
            // Items come by decreasing weight, so the first candidate found is
            // the heaviest one and the last two are the lightest ones
            auto heaviest = -1, lightest = -1, nextLightest = -1;
            for (auto i = 0; i < _nItems; i += 1)
                if (fits(i, b)) {
                    heaviest = heaviest < 0 ? i : heaviest;
                    nextLightest = lightest;
                    lightest = i;
                }

            if (nextLightest >= 0 && _w[lightest] + _w[nextLightest] > _residual[b]) {
                _residual[b] -= _w[heaviest];
                _bin[heaviest]->assign(b);
                someItemAssigned = true;
            }
        }
        return someItemAssigned;
    }

public:
    DecreasingBestFit(CPSolver::Ptr cp, Veci const & bin, vector<int> const & w, var<int>::Ptr nBins, int capacity) :
        _cp(cp), _bin(bin), _w(w), _nBins(nBins), _capacity(capacity), _nItems(scast<int>(bin.size())), _residual(nBins->max() + 1, capacity)
    {
        _binsByResidual.reserve(_residual.size());
    }

    Branches operator()() {
        auto const nBins = _nBins->max() + 1;
        calcResiduals();
        while (perfectFit(nBins) | onlyOneItemFits(nBins)) {}
        _cp->fixpoint();
        auto const unpacked = find_if(_bin.begin(), _bin.end(), [](auto const & x) { return !x->isBound(); });
        if (unpacked == _bin.end())
            return Branches({});
        auto const item = scast<int>(unpacked - _bin.begin());
        auto const weight = _w[item];

        // Select the bin it fills best
        auto bestBin = -1;
        auto bestResidual = highest<int>();
        for (auto b = 0; b < nBins; b += 1)
            if (_bin[item]->contains(b)) {
                auto const residual = _residual[b] - weight;
                if (0 <= residual && residual < bestResidual) {
                    bestBin = b;
                    bestResidual = residual;
                }
            }
        assert(bestBin >= 0);
        auto cp = _cp;
        auto x = _bin[item];
        return Branches({
            [=]() { cp->post(x == bestBin); },
            [this, &cp, &nBins, bestBin, item, weight]() {
                // On refusal, all the bins equivalent to the best one are ruled
                // out, and so are all the items equivalent to the chosen one
                auto const targetResidual = _residual[bestBin];
                for (auto b = 0; b < nBins; b += 1)
                    if (_residual[b] == targetResidual)
                        for (auto j = item; j < _nItems && _w[j] == weight; j += 1)
                            if (!_bin[j]->isBound())
                                _bin[j]->remove(b);
                cp->fixpoint();
            }
        });
    }
};

int main(int argc, char * argv[]) {
    // Parse the command line
    string instanceFile;
    auto timeout = highest<int>(); // ~68 years
    auto gpu = false;

    CLI::App app{"A MiniCPP-based Bin Packing solver"};
    app.add_option("-i", instanceFile, "Instance file path (BPP format)")->check(CLI::ExistingFile)->required();
    app.add_option("-t", timeout, "Timeout in seconds");
    app.add_flag("-g", gpu, "Run the bin packing constraint on the GPU");
    CLI11_PARSE(app, argc, argv);

    auto const [capacity, w] = readInstance(instanceFile);
    auto const nItems = scast<int>(w.size());

    // Bounds on the number of bins
    auto const sum = accumulate(w.begin(), w.end(), 0);
    auto const lb = ceilDiv<int>(sum, capacity);
    auto const ub = lb * 2;

    // Variables
    auto cp = makeSolver();
    auto nBins = makeIntVar(cp, lb - 1, ub);          // The index of the last used bin
    auto bin = intVarArray(cp, nItems, 0, ub);        // The bin of every item
    auto load = intVarArray(cp, ub + 1, 0, capacity); // The load of every bin

    // Only the bins up to the last one can be used
    for (auto const & x : bin)
        cp->post(x <= nBins);

    // Symmetry breaking
    cp->post(lexChainBit(bin));

    // Bin Packing constraint
    cp->post(binPacking(load, bin, w, gpu ? GPU : CPU));

    // Search: pick the number of bins, then fill them
    auto chooseNBins = [=]() { return nBins->isBound() ? Branches({}) : indomain_min(cp, nBins); };
    DecreasingBestFit packItems(cp, bin, w, nBins, capacity);
    DFSearch search(cp, land<function<Branches()>>({chooseNBins, packItems}));

    search.onSolution([&]() {
        cout << "Bins = " << nBins->max() + 1 << "\n"
             << "Packing = " << valueOf(bin) << "\n"
             << "---" << "\n";
    });
    auto limits = [=](SearchStatistics const & ss) {
        return RuntimeMonitor::elapsedSeconds(ss.getStartTime()) > timeout || ss.getSolutions() > 0;
    };
    cout << search.solve(limits) << endl;
    return 0;
}
