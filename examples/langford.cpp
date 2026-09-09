#include "alldifferent.hpp"
#include "lex.hpp"
#include "search.hpp"
#include "utils.hpp"
#include "gfl/Alias.hpp"
#include <CLI/CLI.hpp>

#include <vector>

using namespace std;
using namespace Factory;
using namespace gfl;

// Langford's number problem L(k,n): arrange k copies of each number 1..n in a
// sequence of k*n slots so that the occurrences of m are m slots apart, i.e.
// there are exactly m other numbers between two successive m's.
//
//   L(2,3) = 2 3 1 2 1 3
//
// The model has one variable per number: `first[m]` is the slot of the first
// occurrence of m + 1. The other occurrences follow at a fixed distance, so
// they are offset views of `first[m]` rather than variables of their own, and
// the whole problem is a single alldifferent over the k*n occurrences: no two
// of them may land in the same slot.
//
// That makes it a good workout for the alldifferent propagator -- one dense
// residual graph over k*n occurrences sharing the same k*n slots -- which is
// what the -g flag moves to the GPU.

int main(int argc, char * argv[]) {
    // Parse the command line
    auto n = 12;
    auto k = 2;
    auto timeout = highest<int>(); // ~68 years
    auto allSolutions = false;
    auto gpu = false;

    CLI::App app{"A MiniCPP-based solver for Langford's number problem L(k,n)"};
    app.add_option("-n", n, "How many distinct numbers")->check(CLI::PositiveNumber);
    app.add_option("-k", k, "How many copies of each number")->check(CLI::Range(2, 1000));
    app.add_option("-t", timeout, "Timeout in seconds");
    app.add_flag("-a", allSolutions, "Look for every solution instead of the first one");
    app.add_flag("-g", gpu, "Run the alldifferent constraint on the GPU");
    CLI11_PARSE(app, argc, argv);

    auto const nSlots = n * k;

    auto reportInfeasible = [=]() { cout << "L(" << k << "," << n << ") has no solution\n"; };

    // The copies of the largest number span the most slots, so if they do not
    // fit nothing does
    if (nSlots - 1 - (k - 1) * (n + 1) < 0) {
        reportInfeasible();
        return 0;
    }

    // Variables: the slot of the first occurrence of every number. Number m + 1
    // repeats every m + 2 slots, so its last occurrence pins down the domain
    auto cp = makeSolver();
    auto first = intVarArray(cp, n, [&](int m) { return makeIntVar(cp, 0, nSlots - 1 - (k - 1) * (m + 2)); });

    // Posting the constraints can already prove the instance infeasible
    TRYFAIL
        // The k * n occurrences, each one an offset view of the first one
        auto occurrence = intVarArray(cp, nSlots, [&](int idx) {
            auto const m = idx / k, j = idx % k;
            return j == 0 ? first[m] : first[m] + j * (m + 2);
        });

        // Every occurrence takes a slot of its own
        cp->post(allDifferentAC(occurrence, gpu ? GPU : CPU));

        // Symmetry breaking: a sequence read backwards is a solution as well.
        // The mirror sends slot p to nSlots - 1 - p and reverses the order of
        // the occurrences, so the first occurrence of m + 1 lands where its
        // last one used to be. Keeping the lexicographically smaller of the two
        // vectors rules out exactly one solution of every mirrored pair
        auto mirrored = intVarArray(cp, n, [&](int m) { return (nSlots - 1 - (k - 1) * (m + 2)) - first[m]; });
        cp->post(lexLeq(first, mirrored));
    ONFAIL
        reportInfeasible();
        return 0;
    ENDFAIL

    // Search: the occurrences follow from the first ones, so branch on those
    DFSearch search(cp, firstFail(cp, first));

    search.onSolution([&]() {
        vector<int> sequence(nSlots);
        for (auto m = 0; m < n; m += 1)
            for (auto j = 0; j < k; j += 1)
                sequence[first[m]->min() + j * (m + 2)] = m + 1;
        cout << "Sequence = " << sequence << "\n"
             << "---" << "\n";
    });
    auto limits = [=](SearchStatistics const & ss) {
        return RuntimeMonitor::elapsedSeconds(ss.getStartTime()) > timeout || (not allSolutions && ss.getSolutions() > 0);
    };
    cout << search.solve(limits) << endl;
    return 0;
}
