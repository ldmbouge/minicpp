// Job-shop scheduling in two phases: large neighbourhood search to find good
// schedules, then failure-directed search (FDS) to improve on them and prove
// optimality.
//
// Model: see jobshop.hpp (shared with jobshop.cpp and jobshopLNS.cpp).
//
// Search: andThen(lnsPhase, proofPhase) under optimize, with the time limit
// covering both phases:
//  - lnsPhase is jobshopLNS's search (objcp's mainSubpathLNS): repeat the core
//    search (machines in mainPureCP's order, each sequenced by ect then est,
//    then the makespan) under 3*jobs*machines failures per iteration, freezing
//    every arc outside a random window on 2 random machines of the best
//    solution. It stops after maxStall consecutive restarts without an
//    improvement (counted once there is a first solution);
//  - proofPhase then restarts from the root, keeping the incumbent makespan as
//    an upper bound, and runs FDS on the start times, then labels the makespan.
//    If it completes, the last makespan found is optimal.
//
// Usage:
//   jobshopFDS <file.jss> [timeLimitSeconds (180)] [maxStall (2)] [seed]
// The seed defaults to a random one and is printed, so that a run can be
// reproduced.

#include <iostream>
#include <algorithm>
#include <climits>
#include <cstdlib>
#include <random>
#include <string>
#include <vector>
#include "search.hpp"
#include "RuntimeMonitor.hpp"
#include "range.hpp"
#include "jobshop.hpp"
#include "fds.hpp"

using namespace Factory;

int main(int argc, char* argv[])
{
   if (argc < 2) {
      std::cerr << "usage: " << argv[0] << " <file.jss> [timeLimitSeconds] [maxStall] [seed]\n";
      return 2;
   }
   Instance in(argv[1]);
   if (!in.valid()) {
      std::cerr << "cannot read instance " << argv[1] << "\n";
      return 1;
   }
   double const timeLimit = argc > 2 ? atof(argv[2]) : 180.0;
   int const maxStall = argc > 3 ? atoi(argv[3]) : 2;
   unsigned const seed = argc > 4 ? (unsigned)strtoul(argv[4], nullptr, 10)
                                  : std::random_device{}();
   std::cout << " nbJobs: " << in.nbJobs() << " nbMachines: " << in.nbMachines()
             << " maxStall: " << maxStall << " seed: " << seed << std::endl;

   auto timeStart = RuntimeMonitor::now();
   CPSolver::Ptr cp = makeSolver();
   JobShopModel model(cp, in);
   auto makespan    = model.makespan();
   auto disjunctive = model.disjunctive();
   int const nbMachines = in.nbMachines();

   // Best solution: for every machine, the successor of each node (0 is the
   // source, task t is node t+1, n+1 is the sink), rebuilt from the start
   // times, which are fixed at a leaf.
   std::vector<std::vector<int>> bestSucc(nbMachines);
   auto capture = [&] {
      for (int m = 0; m < nbMachines; m++) {
         auto const& d = disjunctive[m];
         int const n = d->size();
         std::vector<int> order(n);                // tasks of machine m, by start time
         for (int t = 0; t < n; t++) order[t] = t;
         std::stable_sort(order.begin(), order.end(),
                          [&](int a, int b) { return d->est(a) < d->est(b); });
         bestSucc[m].assign(n + 1, n + 1);
         int prev = 0;
         for (int t : order) {
            bestSucc[m][prev] = t + 1;
            prev = t + 1;
         }
         bestSucc[m][prev] = n + 1;
      }
   };

   // As in jobshopLNS: on 2 random machines, fix every arc of the best
   // sequence outside a random window of positions.
   std::mt19937 rng(seed);
   int nbRestarts = 0;
   auto freeze = [&] {
      nbRestarts++;
      std::uniform_int_distribution<int> pickMachine(0, nbMachines - 1);
      std::uniform_int_distribution<int> pickStart(0, in.nbJobs() - 1);
      std::uniform_int_distribution<int> pickLength(2, std::max(2, nbMachines / 5));
      for (int k = 0; k < 2; k++) {
         int const m = pickMachine(rng);
         auto const& succ = disjunctive[m]->successors();
         auto const& sol  = bestSucc[m];
         int const sink = disjunctive[m]->size() + 1;
         int const st = pickStart(rng);
         int const en = st + pickLength(rng);
         for (int j = 0, curr = 0; curr != sink; j++, curr = sol[curr])
            if ((j < st || j >= en) && sol[curr] != sink)
               cp->post(succ[curr] == sol[curr]);
      }
   };

   // Phase 1: LNS around jobshopLNS's core search (the order key is computed
   // in 64 bits: objcp's 32-bit ORInt can overflow once ls >= 2^15).
   auto core = land(
      forall(cp, range(0, nbMachines - 1),
             [&d=disjunctive](int m) {
                return (long long)d[m]->globalSlack() + ((long long)d[m]->localSlack() << 16);
             },
             [&d=disjunctive,&cp](int m) {
                return sequence(cp, d[m],
                                [&](int t) { return d[m]->ect(t); },
                                [&](int t) { return d[m]->est(t); });
             }),
      [&] { return label(cp, makespan); });
   int const maxFailures = 3 * in.nbJobs() * nbMachines;
   auto lnsPhase = lns(cp, capture, freeze, core,
                       [maxFailures](const SearchStatistics& s) {
                          return s.numberOfFailures() >= maxFailures;
                       },
                       maxStall);

   // Phase 2: FDS on the start times from the root, then the makespan.
   bool inProof = false;
   int bestBeforeProof = INT_MAX;
   int best = INT_MAX;
   auto fdsSearch = land(fds(cp, model.allStarts()), [&] { return label(cp, makespan); });
   auto proofPhase = [&, fdsSearch]() -> Branches {
      if (!inProof) {
         inProof = true;
         bestBeforeProof = best;
         std::cout << "LNS stalled after " << nbRestarts << " restarts ("
                   << RuntimeMonitor::elapsedSeconds(timeStart) << " s); FDS from "
                   << (best == INT_MAX ? std::string("no solution") : "makespan " + std::to_string(best))
                   << std::endl;
      }
      return fdsSearch();
   };

   DFSearch search(cp, andThen(cp, lnsPhase, proofPhase));
   int nbFdsImprovements = 0;
   search.onSolution([&]() {
      if (!model.checkSchedule()) {
         std::cerr << "INVALID schedule reported" << std::endl;
         exit(3);
      }
      best = makespan->min();
      if (inProof) nbFdsImprovements++;
      std::cout << "makespan = " << best << "\t(" << RuntimeMonitor::elapsedSeconds(timeStart)
                << " s, " << (inProof ? "FDS" : "LNS restart " + std::to_string(nbRestarts))
                << ")" << std::endl;
   });

   auto obj  = Factory::minimize(makespan);
   auto stat = search.optimize(obj, [&](const SearchStatistics&) {
      return RuntimeMonitor::elapsedSeconds(timeStart) >= timeLimit;
   });

   std::cout << "Time: " << (long)(RuntimeMonitor::elapsedSeconds(timeStart) * 1000) << " ms" << std::endl;
   std::cout << "Restarts: " << nbRestarts << std::endl;
   if (!inProof)
      std::cout << "FDS: not reached (time limit during LNS)" << std::endl;
   else
      std::cout << "FDS: started from " << (bestBeforeProof == INT_MAX ? std::string("no solution")
                                                                        : std::to_string(bestBeforeProof))
                << ", " << nbFdsImprovements << " improvement(s)" << std::endl;
   if (best == INT_MAX)
      std::cout << "Makespan: none found" << std::endl;
   else
      std::cout << "Makespan: " << best << (stat.getCompleted() ? " (optimal)" : "") << std::endl;
   std::cout << stat << std::endl;
   return 0;
}
