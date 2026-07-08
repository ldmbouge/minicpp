#include "scheduling.hpp"
#include "utils.hpp"
#include <nlohmann/json.hpp>
#include <CLI/CLI.hpp>

int main(int argc,char* argv[])
{
    using namespace std;
    using namespace Factory;

    // Arguments
    std::string instance;
    int timeout = numeric_limits<int>::max(); // ~68 years
    std::string searchHeuristic = "SML";
    std::string propagationAlgorithm = "TT";

    // Parsing command line arguments
    CLI::App app{"A MiniCPP-based RCPSP solver"};
    app.add_option("-i", instance,
        "Instance file path")
        ->check(CLI::ExistingFile)->required();
    app.add_option("-t", timeout,
        "Timeout in seconds");
    app.add_option("-s", searchHeuristic ,
        "Search heuristic")
        ->check(CLI::IsMember({"FF", "SML"}));
    app.add_option("-a", propagationAlgorithm,
        "Cumulative propagation algorithm")
        ->check(CLI::IsMember({"TT", "ER", "ER-GPU", "TT+ER-GPU"}));

    CLI11_PARSE(app, argc, argv);

    // Read instance
    auto const data = nlohmann::json::parse(ifstream(instance));

    // Resources
    int const nRes = data["n_res"];     // The number of resources
    vector<int> const rc = data["rc"];  // The resource capabilities

    // Tasks
    int const nTasks = data["n_tasks"];          // The number of tasks
    vector<int> const d = data["d"];             // The task durations
    vector<vector<int>> const rr = data["rr"];   // The resource requirements
    vector<vector<int>> const suc = data["suc"]; // The task successors

    // Planning horizon
    int const tMax = std::accumulate(d.begin(), d.end(), 0); // End time of the planning horizon

    // Variables
    auto cp = makeSolver();
    auto st = intVarArray(cp,nTasks,0,tMax - 1);  // The tasks start time
    auto makespan = makeIntVar(cp,0,tMax);     // The project duration

    // Precedence constraints
    for (int i = 0; i < nTasks; i += 1)
       for (auto const & j : suc[i])
          cp->post(st[i] + d[i]<= st[j]);

    // Redundant non-overlapping constraints
    for (int i = 0; i < nTasks; i += 1)
        for (int j = i+1; j < nTasks; j += 1)
            for (int r = 0; r < nRes; r += 1)
                if (rr[r][i] + rr[r][j] > rc[r]) {
                    auto ij = isLessOrEqual(st[i] + d[i], st[j]);
                    auto ji = isLessOrEqual(st[j] + d[j], st[i]);
                    cp->post(clause({ij,ji}));
                }

    // Cumulative resource constraints
    for( int i = 0; i < nRes; i += 1) {
        if (propagationAlgorithm == "TT") {
            cp->post(cumulativeTT(st, d, rr[i], rc[i], CDevice::CPU));
        } else if (propagationAlgorithm == "ER") {
            cp->post(cumulativeER(st, d, rr[i], rc[i], CDevice::CPU));
        } else if (propagationAlgorithm == "ER-GPU") {
            cp->post(cumulativeER(st, d, rr[i], rc[i], CDevice::GPU));
        } else if (propagationAlgorithm == "TT+ER-GPU") {
            cp->post(cumulativeTT(st, d, rr[i], rc[i], CDevice::CPU));
            cp->post(cumulativeER(st, d, rr[i], rc[i], CDevice::GPU));
        }
    }

    // Makespan constraints
    for (int i = 0; i < nTasks; i += 1)
       if (suc[i].empty())
          cp->post(st[i] + d[i] <= makespan);

    auto vars = st;
    vars.push_back(makespan);

    // Search
    auto heuristic = searchHeuristic == "FF" ? firstFail(cp, vars)
                                             : smallest(cp, vars);
    DFSearch search(cp, std::move(heuristic));
    search.onSolution([&st,&makespan]() {
        std::cout << "Makespan = " << makespan->min() << "\n"
                  << "Starting Times = " << valueOf(st) << "\n"
                  << "---"<< "\n";
    });

    auto obj = minimize(makespan);
    auto limits = [=](SearchStatistics const & ss) {
        return  RuntimeMonitor::elapsedSeconds(ss.getStartTime()) > timeout;
    };
    auto stat = search.optimize(obj, limits);
    cout << stat << endl;
    return 0;
}
