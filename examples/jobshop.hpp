// Job-shop model shared by jobshop.cpp (pure CP) and jobshopLNS.cpp (LNS),
// ports of Objective-CP's Clo/Scheduler/Tests/jobshop/main.m
// (Copyright (c) 2015 NICTA, Pascal Van Hentenryck, MPL 2.0).
//
// Model: one start variable per operation, job precedences, makespan >= end
// of every job, and one DisjunctiveResource per machine: operations are added
// to their machine's resource, then posting the resource imposes Disjunctive
// and TaskSequence (successor variables) on them.
//
// .jss format: nbJobs nbMachines, then per job nbMachines pairs (machine dur),
// machines numbered from 0, operations in job order.

#pragma once

#include <fstream>
#include <string>
#include <vector>
#include "solver.hpp"
#include "intvar.hpp"
#include "constraint.hpp"
#include "scheduling.hpp"
#include "utils.hpp"

// A job-shop instance read from a .jss file. Check valid() after construction.
class Instance {
   int                           _nbJobs = 0;
   int                           _nbMachines = 0;
   std::vector<std::vector<int>> _machine;    // [job][op] machine, 0-based
   std::vector<std::vector<int>> _duration;   // [job][op]
   bool                          _valid = false;
public:
   explicit Instance(const std::string& fn)
   {
      std::ifstream f(fn);
      if (!(f >> _nbJobs >> _nbMachines) || _nbJobs <= 0 || _nbMachines <= 0)
         return;
      _machine.assign(_nbJobs, std::vector<int>(_nbMachines));
      _duration.assign(_nbJobs, std::vector<int>(_nbMachines));
      for (int i = 0; i < _nbJobs; i++)
         for (int j = 0; j < _nbMachines; j++) {
            if (!(f >> _machine[i][j] >> _duration[i][j]))
               return;
            if (_machine[i][j] < 0 || _machine[i][j] >= _nbMachines || _duration[i][j] < 0)
               return;
         }
      _valid = true;
   }
   // True when the file was read completely and its data is consistent:
   // positive sizes, machines in [0, nbMachines), non-negative durations.
   bool valid() const { return _valid; }
   int  nbJobs() const { return _nbJobs; }
   int  nbMachines() const { return _nbMachines; }
   int  machine(int job, int op) const { return _machine[job][op]; }
   int  duration(int job, int op) const { return _duration[job][op]; }
   int  totalDuration() const { return sum(_duration, [](const auto& job) { return sum(job); }); }
};

// The CP model of an instance: start variables, makespan and one disjunctive
// resource per machine, all posted on construction.
class JobShopModel {
   const Instance&                           _in;
   CPSolver::Ptr                             _cp;
   Matrix<var<int>::Ptr,2>                   _start;         // [job][op]
   var<int>::Ptr                             _makespan;
   std::vector<DisjunctiveResource::Ptr>     _disjunctive;   // [machine]
public:
   JobShopModel(CPSolver::Ptr cp, const Instance& in)
      : _in(in), _cp(cp),
        _start(Factory::intVarMatrix(cp, in.nbJobs(), in.nbMachines(), [&](int i, int j) {
           return Factory::makeIntVar(cp, 0, in.totalDuration() - in.duration(i, j));
        })),
        _makespan(Factory::makeIntVar(cp, 0, in.totalDuration())),
        _disjunctive(Factory::disjunctiveArray(cp, in.nbMachines()))
   {
      using namespace Factory;
      // job precedences and makespan
      for (int i = 0; i < in.nbJobs(); i++) {
         for (int j = 0; j + 1 < in.nbMachines(); j++)
            cp->post(_start[i][j] + in.duration(i, j) <= _start[i][j + 1]);
         int const last = in.nbMachines() - 1;
         cp->post(_start[i][last] + in.duration(i, last) <= _makespan);
      }
      // machines: one disjunctive resource each. Associate every operation
      // with its machine, then impose the disjunctive requirements on those tasks.
      for (int i = 0; i < in.nbJobs(); i++)
         for (int j = 0; j < in.nbMachines(); j++)
            _disjunctive[in.machine(i, j)]->add(_start[i][j], in.duration(i, j));
      for (auto& d : _disjunctive)
         cp->post(d);
   }
   // Start variable of operation op of job (Matrix has no const subscript;
   // its storage is row-major).
   var<int>::Ptr start(int job, int op) const { return _start.flat()[job * _in.nbMachines() + op]; }
   var<int>::Ptr                                makespan() const { return _makespan; }
   const std::vector<DisjunctiveResource::Ptr>& disjunctive() const { return _disjunctive; }

   // Independent check of a solution: earliest starts must satisfy every
   // precedence, never overlap on a machine, and end by the makespan.
   bool checkSchedule() const
   {
      for (int i = 0; i < _in.nbJobs(); i++)
         for (int j = 0; j < _in.nbMachines(); j++) {
            int const e = start(i, j)->min() + _in.duration(i, j);
            if (j + 1 < _in.nbMachines() ? e > start(i, j + 1)->min() : e > _makespan->min())
               return false;
         }
      for (auto& d : _disjunctive)
         for (int a = 0; a < d->size(); a++)
            for (int b = a + 1; b < d->size(); b++) {
               if (d->duration(a) == 0 || d->duration(b) == 0) continue;
               if (d->est(a) < d->ect(b) && d->est(b) < d->ect(a)) return false;
            }
      return true;
   }
};
