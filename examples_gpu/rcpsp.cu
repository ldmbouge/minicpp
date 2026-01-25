#include <iostream>
#include <fstream>
#include <iomanip>
#include "solver.hpp"
#include "intvar.hpp"
#include "constraint.hpp"
#include "search.hpp"
#include <nlohmann/json.hpp>
#include <fmt/base.h>
#include <fmt/ranges.h>

#include "gpu_constriants/cumulative.cuh"

int main(int argc,char* argv[])
{
    using namespace std;
    using namespace fmt;
    using namespace Factory;

    using json = nlohmann::json;
    using IntVar = var<int>::Ptr;

    // Model based on https://github.com/MiniZinc/minizinc-benchmarks/blob/master/rcpsp/rcpsp.mzn

    ifstream f(argv[1]);
    auto const data = json::parse(f);
    // cout << data.dump() << std::endl;

    // Resources
    int const n_res = data["n_res"];    // The number of resources
    vector<int> const rc = data["rc"];  // The resource capabilities
    // println("n_res = {}",n_res);
    // println("rc = {}",rc);

    // Tasks
    int const n_tasks = data["n_tasks"];         // The number of tasks
    vector<int> const d = data["d"];             // The task durations
    vector<vector<int>> const rr = data["rr"];   // The resource requirements
    vector<vector<int>> const suc = data["suc"]; // The task successors
    // println("n_tasks = {}", n_tasks);
    // println("d = {}", d);
    // println("rr = {}", rr);
    // println("suc = {}", suc);

    // Planning horizon
    int const t_max = std::accumulate(d.begin(), d.end(), 0); // End time of the planning horizon
    int const est = 0;                                                    // Earliest starting time
    int const lst = t_max - 1;                                            // Latest starting time

    // Variables
    auto cp = makeSolver();
    auto st = makeIntVars(cp,n_tasks,est,lst);  // The tasks start time
    auto makespan = makeIntVar(cp,0,t_max);                            // The project duration

    // Precedence constraints
    for (int i = 0; i < n_tasks; i += 1)
    {
        for (auto const & j : suc[i])
        {
            cp->post(st[i] + d[i]<= st[j]);
        }
    }

    // Cumulative resource constraints
    for( int i = 0; i < n_res; i += 1)
    {
        //cp->post(new (cp) Cumulative(st,d,rr[i],rc[i]));
        cp->post(new (cp) CumulativeGPU(st,d,rr[i],rc[i]));
    }

    // Makespan constraints
    for (int i = 0; i < n_tasks; i += 1)
    {
       if (suc[i].empty())
       {
           cp->post(st[i] + d[i] <= makespan);
       }
    }

    // Search
    DFSearch search(cp,[&cp,&st,&makespan]() {

        // Variable selection
        auto const isNotBounded = [](const auto & x) {return x->size() > 1;};
        auto const smallestValue = [](const auto & x) {return x->min();};
        auto vars = st;
        vars.push_back(makespan);
        auto const var = selectMin(vars,isNotBounded, smallestValue);

        if (var)
        {
            // Value selection
            int const val = smallestValue(var);
            std::vector<function<void(void)>> br;
            br.push_back([cp,var,val] { return cp->post(var == val);});
            br.push_back([cp,var,val] { return cp->post(var != val);});
            return Branches(br);
        }
        else
            return Branches({});
    });

    search.onSolution([&st,&makespan]() {
        println("Makespan = {}", makespan->min());
        print("Starting Times = [");
        for (int i = 0; i < st.size(); i += 1)
        {
            print(i == 0 ? "{}" : ",{}", st[i]->min());
        }
        println("]");
        println("---");
    });

    auto obj = minimize(makespan);
    auto stat = search.optimize(obj);
    cout << stat << endl;

    return 0;
}
