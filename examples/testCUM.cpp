#include "scheduling.hpp"
#include <nlohmann/json.hpp>

int main(int argc,char* argv[])
{
    using namespace std;
    using namespace Factory;
    using json = nlohmann::json;
    ifstream f(argv[1]);
    auto const data = json::parse(f);
    // Resources
    int const nRes = data["n_res"];    // The number of resources
    vector<int> const rc = data["rc"];  // The resource capabilities
    // Tasks
    int const nTasks = data["n_tasks"];         // The number of tasks
    vector<int> const d = data["d"];             // The task durations
    vector<vector<int>> const rr = data["rr"];   // The resource requirements
    vector<vector<int>> const suc = data["suc"]; // The task successors
    // Planning horizon
    int const tMax = std::accumulate(d.begin(), d.end(), 0); // End time of the planning horizon
    // Variables
    auto cp = makeSolver();
    auto st = intVarArray(cp,nTasks,0,tMax - 1);  // The tasks start time
    auto makespan = makeIntVar(cp,0,tMax);        // The project duration

    // Precedence constraints
    for (int i = 0; i < nTasks; i += 1)
       for (auto const & j : suc[i])
          cp->post(st[i] + d[i]<= st[j]);

    // Cumulative resource constraints
    for( int i = 0; i < nRes; i += 1)
       cp->post(cumulativeEN(st,d,rr[i],rc[i],GPU));          

    // Makespan constraints
    for (int i = 0; i < nTasks; i += 1)
       if (suc[i].empty())
          cp->post(st[i] + d[i] <= makespan);

    auto vars = st;
    vars.push_back(makespan);
    // Search
    DFSearch search(cp,[&cp,&vars]() {
        // Variable selection
        auto const var = selectMin(vars,
                                   [](const auto &x) { return x->size() > 1; },
                                   [](const auto &x) { return x->min(); }
                                   );
        if (var) { // Value selection
            int const val = var->min();
            std::vector<function<void(void)>> br;
            br.push_back([cp,var,val] { return cp->post(var == val);});
            br.push_back([cp,var,val] { return cp->post(var != val);});
            return Branches(br);
        }
        else
            return Branches({});
    });

    search.onSolution([&st,&makespan]() {
      std::cout << "Makespan = " << makespan->min() << "\n";
      std::cout << "Starting Times = [";
      for (auto i = 0u; i < st.size(); i += 1) {
         if (i!= 0) std::cout << ',';
         std::cout << st[i]->min();
      }
      std::cout << "]" << "\n";
      std::cout << "---" << "\n";
    });

    auto obj = minimize(makespan);
    auto stat = search.optimize(obj);
    cout << stat << endl;

    return 0;
}
