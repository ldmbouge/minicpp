// Job-shop scheduling by large neighbourhood search: port of mainSubpathLNS
// from Objective-CP's Clo/Scheduler/Tests/jobshop/main.m (Copyright (c) 2015
// NICTA, Pascal Van Hentenryck, MPL 2.0).
//
// Model: see jobshop.hpp (shared with jobshop.cpp).
//
// Search: lns(capture, freeze, core, iterationLimit), i.e. objcp's
//    limitTime: T in: repeat: { limitFailures: 3*jobs*machines in: core }
//                     onRepeat: { freeze }
// where
//  - core is the pure CP search: forall machines orderedBy
//    globalSlack + (localSlack << 16) (as in mainPureCP): sequence the machine
//    by smallest ect, then est (as in mainSubpathLNS); then label the makespan;
//  - capture records the successors of the solution;
//  - freeze, as in mainSubpathLNS: on 2 random machines, pick a window of
//    positions [st, st+len) along the machine's sequence in the best solution,
//    len in [2, max(2, machines/5)], and fix every arc outside the window.
//    That is what objcp actually runs: its test meant to spare tight arcs
//    reads `est = [sol ect: t[n]]`, so it never filters, and it is dropped.
//
// Usage:
//   jobshopLNS <file.jss> [timeLimitSeconds (180)] [seed]
// The seed defaults to a random one (objcp calls setRandomized) and is printed,
// so that a run can be reproduced.

#include <iostream>
#include <climits>
#include <cstdlib>
#include <random>
#include <string>
#include <vector>
#include "search.hpp"
#include "RuntimeMonitor.hpp"
#include "range.hpp"
#include "jobshop.hpp"

using namespace Factory;

int main(int argc, char* argv[])
{
   if (argc < 2) {
      std::cerr << "usage: " << argv[0] << " <file.jss> [timeLimitSeconds] [seed]\n";
      return 2;
   }
   Instance in(argv[1]);
   if (!in.valid()) {
      std::cerr << "cannot read instance " << argv[1] << "\n";
      return 1;
   }
   double const timeLimit = argc > 2 ? atof(argv[2]) : 180.0;
   unsigned const seed = argc > 3 ? (unsigned)strtoul(argv[3], nullptr, 10)
                                  : std::random_device{}();
   std::cout << " nbJobs: " << in.nbJobs() << " nbMachines: " << in.nbMachines()
             << " seed: " << seed << std::endl;

   auto timeStart = RuntimeMonitor::now();
   CPSolver::Ptr cp = makeSolver();
   JobShopModel model(cp, in);
   auto makespan    = model.makespan();
   auto disjunctive = model.disjunctive();
   int const nbMachines = in.nbMachines();

   // Best solution: for every machine, the successor of each node (0 is the
   // source, 1..n the tasks, n+1 the sink).
   std::vector<std::vector<int>> bestSucc(nbMachines);
   auto capture = [&] {
      for (int m = 0; m < nbMachines; m++) {
         auto const& succ = disjunctive[m]->successors();
         bestSucc[m].resize(succ.size());
         for (size_t k = 0; k < succ.size(); k++)
            bestSucc[m][k] = succ[k]->min();       // all bound at a leaf
      }
   };

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
         // Walk the machine's sequence in the best solution; j is the position.
         for (int j = 0, curr = 0; curr != sink; j++, curr = sol[curr])
            if ((j < st || j >= en) && sol[curr] != sink)
               cp->post(succ[curr] == sol[curr]);
      }
   };

   // Core search: machines in mainPureCP's order, each sequenced as in
   // mainSubpathLNS (by ect, then est), then the makespan. The order key is
   // computed in 64 bits: objcp's 32-bit ORInt can overflow once ls >= 2^15.
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
   DFSearch search(cp, lns(cp, capture, freeze, core,
                           [maxFailures](const SearchStatistics& s) {
                              return s.numberOfFailures() >= maxFailures;
                           }));

   int best = INT_MAX;
   search.onSolution([&]() {
      if (!model.checkSchedule()) {
         std::cerr << "INVALID schedule reported" << std::endl;
         exit(3);
      }
      best = makespan->min();
      std::cout << "makespan = " << best << "\t(" << RuntimeMonitor::elapsedSeconds(timeStart)
                << " s, restart " << nbRestarts << ")" << std::endl;
   });

   // objcp's outer limitTime: the search-wide Limit.
   auto obj  = Factory::minimize(makespan);
   auto stat = search.optimize(obj, [&](const SearchStatistics&) {
      return RuntimeMonitor::elapsedSeconds(timeStart) >= timeLimit;
   });

   std::cout << "Time: " << (long)(RuntimeMonitor::elapsedSeconds(timeStart) * 1000) << " ms" << std::endl;
   std::cout << "Restarts: " << nbRestarts << std::endl;
   if (best == INT_MAX)
      std::cout << "Makespan: none found" << std::endl;
   else
      std::cout << "Makespan: " << best << std::endl;
   std::cout << stat << std::endl;
   return 0;
}
