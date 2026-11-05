// Job-shop scheduling: port of mainPureCP from Objective-CP's
// Clo/Scheduler/Tests/jobshop/main.m (Copyright (c) 2015 NICTA,
// Pascal Van Hentenryck, MPL 2.0).
//
// Model: one start variable per operation, job precedences, makespan >= end
// of every job, and one DisjunctiveResource per machine: operations are added
// to their machine's resource, then posting the resource imposes Disjunctive
// and TaskSequence (successor variables) on them.
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
// .jss format: nbJobs nbMachines, then per job nbMachines pairs (machine dur),
// machines numbered from 0, operations in job order.
//
// The two instances hard-coded in main.m are data/jsp/ft06v.jss (ft06 with
// job 6's last operation lasting 2) and data/jsp/ft10small.jss (ft10's
// routing with durations about ft10/10, one of them 0). Its commented-out
// 10x10 instances are ft10, orb01 and orb02.

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <climits>
#include <cstdlib>
#include "solver.hpp"
#include "intvar.hpp"
#include "constraint.hpp"
#include "search.hpp"
#include "scheduling.hpp"
#include "RuntimeMonitor.hpp"
#include "range.hpp"

using namespace Factory;

// A job-shop instance read from a .jss file. Check valid() after construction.
class Instance {
   bool _valid = false;
public:
   int nbJobs = 0, nbMachines = 0;
   std::vector<std::vector<int>> resource;   // [job][op] machine, 0-based
   std::vector<std::vector<int>> duration;   // [job][op]

   explicit Instance(const std::string& fn)
   {
      std::ifstream f(fn);
      if (!(f >> nbJobs >> nbMachines) || nbJobs <= 0 || nbMachines <= 0)
         return;
      resource.assign(nbJobs, std::vector<int>(nbMachines));
      duration.assign(nbJobs, std::vector<int>(nbMachines));
      for (int i = 0; i < nbJobs; i++)
         for (int j = 0; j < nbMachines; j++) {
            if (!(f >> resource[i][j] >> duration[i][j]))
               return;
            if (resource[i][j] < 0 || resource[i][j] >= nbMachines || duration[i][j] < 0)
               return;
         }
      _valid = true;
   }
   // True when the file was read completely and its data is consistent:
   // positive sizes, machines in [0, nbMachines), non-negative durations.
   bool valid() const { return _valid; }
};

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
   std::cout << " nbJobs: " << in.nbJobs << " nbMachines: " << in.nbMachines << std::endl;

   auto timeStart = RuntimeMonitor::now();
   CPSolver::Ptr cp = makeSolver();

   int const totalDuration = sum(in.duration, [](const auto& job) { return sum(job); });

   // variables: start of operation (job i, op j), within the horizon
   auto start = intVarMatrix(cp, in.nbJobs, in.nbMachines, [&](int i, int j) {
       return makeIntVar(cp, 0, totalDuration - in.duration[i][j]);
   });
   auto makespan = makeIntVar(cp, 0, totalDuration);

   // job precedences and makespan
   for (int i = 0; i < in.nbJobs; i++) {
      for (int j = 0; j + 1 < in.nbMachines; j++)
         cp->post(start[i][j] + in.duration[i][j] <= start[i][j + 1]);
      int const last = in.nbMachines - 1;
      cp->post(start[i][last] + in.duration[i][last] <= makespan);
   }

   // machines: one disjunctive resource each. Associate every operation with
   // its machine, then impose the disjunctive requirements on those tasks.
   auto disjunctive = disjunctiveArray(cp, in.nbMachines);
   for (int i = 0; i < in.nbJobs; i++)
      for (int j = 0; j < in.nbMachines; j++)
         disjunctive[in.resource[i][j]]->add(start[i][j], in.duration[i][j]);
   for (auto& d : disjunctive)
      cp->post(d);

   // Search, as in objcp's mainPureCP:
   //    forall: Machines orderedBy: gs + (ls << 16) do:
   //       sequence: successors by: est then: ect
   //    label: makespan
   // The order key is computed in 64 bits: objcp's 32-bit ORInt can overflow
   // once ls >= 2^15.
   DFSearch search(cp, land(
      forall(cp, range(0, in.nbMachines - 1),
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

   // Independent check of a solution: earliest starts must satisfy every
   // precedence, never overlap on a machine, and end by the makespan.
   auto checkSchedule = [&]() {
      for (int i = 0; i < in.nbJobs; i++)
         for (int j = 0; j < in.nbMachines; j++) {
            int const e = start[i][j]->min() + in.duration[i][j];
            if (j + 1 < in.nbMachines ? e > start[i][j + 1]->min() : e > makespan->min())
               return false;
         }
      for (auto& d : disjunctive)
         for (int a = 0; a < d->size(); a++)
            for (int b = a + 1; b < d->size(); b++) {
               if (d->duration(a) == 0 || d->duration(b) == 0) continue;
               if (d->est(a) < d->ect(b) && d->est(b) < d->ect(a)) return false;
            }
      return true;
   };

   int best = INT_MAX;
   search.onSolution([&]() {
      if (!checkSchedule()) {
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
