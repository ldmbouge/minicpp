// Job-shop scheduling: port of mainPureCP from Objective-CP's
// Clo/Scheduler/Tests/jobshop/main.m (Copyright (c) 2015 NICTA,
// Pascal Van Hentenryck, MPL 2.0).
//
// Model: see jobshop.hpp (shared with jobshopLNS.cpp).
//
// Search (same as the original):
//   forall machines orderedBy globalSlack + (localSlack << 16) (re-evaluated
//   each time a machine is done, as ORControl's forall:orderedBy: does):
//      sequence the machine: repeatedly take the first unbound successor on its
//      path and label it, trying tasks by smallest est, ties by smallest ect;
//   then label the makespan.
// Branch-and-bound minimises the makespan.
//
// Usage:
//   jobshop <file.jss> [timeLimitSeconds]
//
// The two instances hard-coded in main.m are data/jsp/ft06v.jss (ft06 with
// job 6's last operation lasting 2) and data/jsp/ft10small.jss (ft10's
// routing with durations about ft10/10, one of them 0). Its commented-out
// 10x10 instances are ft10, orb01 and orb02.

#include <iostream>
#include <climits>
#include <cstdlib>
#include "search.hpp"
#include "RuntimeMonitor.hpp"
#include "range.hpp"
#include "jobshop.hpp"

using namespace Factory;

int main(int argc, char* argv[])
{
   if (argc < 2) {
      std::cerr << "usage: " << argv[0] << " <file.jss> [timeLimitSeconds]\n";
      return 2;
   }
   Instance in(argv[1]);
   if (!in.valid()) {
      std::cerr << "cannot read instance " << argv[1] << "\n";
      return 1;
   }
   double const timeLimit = argc > 2 ? atof(argv[2]) : 0.0;   // 0: no limit
   std::cout << " nbJobs: " << in.nbJobs() << " nbMachines: " << in.nbMachines() << std::endl;

   auto timeStart = RuntimeMonitor::now();
   CPSolver::Ptr cp = makeSolver();

   JobShopModel model(cp, in);
   auto makespan    = model.makespan();
   auto disjunctive = model.disjunctive();

   // Search, as in objcp's mainPureCP:
   //    forall: Machines orderedBy: gs + (ls << 16) do:
   //       sequence: successors by: est then: ect
   //    label: makespan
   // The order key is computed in 64 bits: objcp's 32-bit ORInt can overflow
   // once ls >= 2^15.
   DFSearch search(cp, land(
      forall(cp, range(0, in.nbMachines() - 1),
             [&d=disjunctive](int m) {
                return (long long)d[m]->globalSlack() + ((long long)d[m]->localSlack() << 16);
             },
             [&d=disjunctive,&cp](int m) {
                return sequence(cp, d[m],
                                [&](int t) { return d[m]->est(t); },
                                [&](int t) { return d[m]->ect(t); });
             }),
      [&] { return label(cp, makespan); }
   ));

   int best = INT_MAX;
   search.onSolution([&]() {
      if (!model.checkSchedule()) {
         std::cerr << "INVALID schedule reported" << std::endl;
         exit(3);
      }
      best = makespan->min();
      std::cout << "makespan = [" << makespan->min() << "," << makespan->max() << "]"
                << "\t(" << RuntimeMonitor::elapsedSeconds(timeStart) << " s)" << std::endl;
   });

   auto obj = Factory::minimize(makespan);
   SearchStatistics stat;
   if (timeLimit > 0)
      stat = search.optimize(obj, [&](const SearchStatistics&) {
         return RuntimeMonitor::elapsedSeconds(timeStart) > timeLimit;
      });
   else
      stat = search.optimize(obj);

   std::cout << "Time: " << (long)(RuntimeMonitor::elapsedSeconds(timeStart) * 1000) << " ms" << std::endl;
   if (best == INT_MAX)
      std::cout << "Makespan: none found" << std::endl;
   else
      std::cout << "Makespan: " << best << (stat.getCompleted() ? " (optimal)" : " (limit reached)") << std::endl;
   std::cout << stat << std::endl;
   return 0;
}
