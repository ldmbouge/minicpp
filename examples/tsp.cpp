/*
 * mini-cp is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License  v3
 * as published by the Free Software Foundation.
 *
 * mini-cp is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY.
 * See the GNU Lesser General Public License  for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with mini-cp. If not, see http://www.gnu.org/licenses/lgpl-3.0.en.html
 *
 * Copyright (c)  2018. by Laurent Michel, Pierre Schaus, Pascal Van Hentenryck
 */

#include "circuit.hpp"
#include "search.hpp"
#include "utils.hpp"
#include "gfl/Alias.hpp"
#include <CLI/CLI.hpp>

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace std;
using namespace Factory;
using namespace gfl;

// The travelling salesman problem over a TSPLIB instance: `succ[i]` is the city
// visited right after city `i`, a circuit constraint makes those successors one
// single tour through every city, and the objective sums the leg lengths.
//
// The circuit rests on an alldifferent over the successors -- no two cities may
// have the same one -- which is what the -g flag moves to the GPU.

namespace {

// TSPLIB rounds to the nearest integer this way, ties going up
int nint(double x) { return static_cast<int>(x + 0.5); }

// TSPLIB states geographical coordinates as DDD.MM, degrees and minutes packed
// into one number, and wants them in radians. The spec truncates the degrees
// rather than rounding them, and spells out its own value for pi -- both are
// load-bearing, since matching the published optimal tour lengths on the GEO
// instances takes the exact same arithmetic
double toRadians(double ddd_mm) {
    auto const PI = 3.141592;
    auto const deg = static_cast<double>(static_cast<int>(ddd_mm));
    auto const min = ddd_mm - deg;
    return PI * (deg + 5.0 * min / 3.0) / 180.0;
}

// The distance functions of the TSPLIB spec, section 2.1
int distance(string const & type, vector<double> const & x, vector<double> const & y, int i, int j) {
    auto const xd = x[i] - x[j], yd = y[i] - y[j];
    if (type == "EUC_2D")  return nint(sqrt(xd * xd + yd * yd));
    if (type == "CEIL_2D") return static_cast<int>(ceil(sqrt(xd * xd + yd * yd)));
    if (type == "MAN_2D")  return nint(fabs(xd) + fabs(yd));
    if (type == "MAX_2D")  return max(nint(fabs(xd)), nint(fabs(yd)));
    if (type == "ATT") {
        // Pseudo-Euclidean, rounded away from zero
        auto const r = sqrt((xd * xd + yd * yd) / 10.0);
        auto const t = nint(r);
        return t < r ? t + 1 : t;
    }
    if (type == "GEO") {
        // x holds the latitudes and y the longitudes, both already in radians
        auto const q1 = cos(y[i] - y[j]);
        auto const q2 = cos(x[i] - x[j]);
        auto const q3 = cos(x[i] + x[j]);
        auto const RRR = 6378.388;
        return static_cast<int>(RRR * acos(0.5 * ((1.0 + q1) * q2 - (1.0 - q1) * q3)) + 1.0);
    }
    throw runtime_error("Unsupported EDGE_WEIGHT_TYPE: " + type);
}

// Fill in the half of an EXPLICIT matrix the file leaves out. Every TSPLIB
// EDGE_WEIGHT_FORMAT other than FULL_MATRIX gives one triangle only, and the
// instances that use them are symmetric
void mirror(vector<vector<int>> & d) {
    auto const n = d.size();
    for (auto i = 0u; i < n; i += 1)
        for (auto j = i + 1; j < n; j += 1)
            if (d[i][j] == 0)
                d[i][j] = d[j][i];
            else
                d[j][i] = d[i][j];
}

// Read a TSPLIB instance: the specification part is a series of `KEY : VALUE`
// lines, followed by the data part. Returns the full distance matrix, whether
// the file gave the weights outright or the coordinates to derive them from.
vector<vector<int>> readInstance(string const & path) {
    ifstream file(path);
    if (!file)
        throw runtime_error("Cannot open " + path);

    auto n = 0;
    string type = "EUC_2D", format = "FUNCTION";
    vector<vector<int>> d;
    vector<double> cx, cy;

    string line;
    while (getline(file, line)) {
        // Strip the trailing carriage return of a DOS-formatted instance
        while (!line.empty() && (line.back() == '\r' || line.back() == ' '))
            line.pop_back();
        if (line.empty())
            continue;

        auto const colon = line.find(':');
        auto const keyword = line.substr(0, colon == string::npos ? line.find(' ') : colon);
        auto key = keyword.substr(0, keyword.find_last_not_of(" \t") + 1);
        auto value = colon == string::npos ? string() : line.substr(colon + 1);
        value.erase(0, value.find_first_not_of(" \t"));
        value.erase(value.find_last_not_of(" \t") + 1);

        if (key == "DIMENSION")               n = stoi(value);
        else if (key == "EDGE_WEIGHT_TYPE")   type = value;
        else if (key == "EDGE_WEIGHT_FORMAT") format = value;
        else if (key == "NODE_COORD_SECTION") {
            cx.resize(n);
            cy.resize(n);
            for (auto i = 0; i < n; i += 1) {
                int id;
                file >> id >> cx[i] >> cy[i];
                if (type == "GEO") {
                    cx[i] = toRadians(cx[i]);
                    cy[i] = toRadians(cy[i]);
                }
            }
        }
        else if (key == "EDGE_WEIGHT_SECTION") {
            d.assign(n, vector<int>(n, 0));
            if (format == "FULL_MATRIX")
                for (auto i = 0; i < n; i += 1)
                    for (auto j = 0; j < n; j += 1)
                        file >> d[i][j];
            else if (format == "UPPER_ROW")
                for (auto i = 0; i < n; i += 1)
                    for (auto j = i + 1; j < n; j += 1)
                        file >> d[i][j];
            else if (format == "UPPER_DIAG_ROW")
                for (auto i = 0; i < n; i += 1)
                    for (auto j = i; j < n; j += 1)
                        file >> d[i][j];
            else if (format == "LOWER_ROW")
                for (auto i = 0; i < n; i += 1)
                    for (auto j = 0; j < i; j += 1)
                        file >> d[i][j];
            else if (format == "LOWER_DIAG_ROW")
                for (auto i = 0; i < n; i += 1)
                    for (auto j = 0; j <= i; j += 1)
                        file >> d[i][j];
            else
                throw runtime_error("Unsupported EDGE_WEIGHT_FORMAT: " + format);
            if (format != "FULL_MATRIX")
                mirror(d);
        }
        else if (key == "EOF")
            break;
    }

    if (n <= 0)
        throw runtime_error("No DIMENSION in " + path);
    if (d.empty()) {
        if (cx.empty())
            throw runtime_error("Neither weights nor coordinates in " + path);
        d.assign(n, vector<int>(n, 0));
        for (auto i = 0; i < n; i += 1)
            for (auto j = 0; j < n; j += 1)
                if (i != j)
                    d[i][j] = distance(type, cx, cy, i, j);
    }
    return d;
}

} // namespace

int main(int argc, char * argv[])
{
    // Parse the command line
    string instanceFile;
    auto timeout = highest<int>(); // ~68 years
    auto showMatrix = false;
    auto gpu = false;

    CLI::App app{"A MiniCPP-based TSP solver"};
    app.add_option("-i", instanceFile, "Instance file path (TSPLIB format)")->check(CLI::ExistingFile)->required();
    app.add_option("-t", timeout, "Timeout in seconds");
    app.add_flag("-p", showMatrix, "Print the distance matrix");
    app.add_flag("-g", gpu, "Run the alldifferent behind the circuit constraint on the GPU");
    CLI11_PARSE(app, argc, argv);

    auto const d = readInstance(instanceFile);
    auto const n = static_cast<int>(d.size());

    Matrix<int,2> distanceMatrix({n,n});
    auto longestLeg = 0;
    for(int i =0;i < n;i++)
       for(int j=0;j < n;j++) {
          distanceMatrix[i][j] = d[i][j];
          longestLeg = max(longestLeg, d[i][j]);
       }

    if (showMatrix)
       for(int i =0;i < n;i++) {
          for(int j=0;j < n;j++)
             cout << setw(6) << distanceMatrix[i][j] << " ";
          cout << endl;
       }

    CPSolver::Ptr cp  = Factory::makeSolver();
    auto succ = Factory::intVarArray(cp,n,n);
    auto distSucc = Factory::intVarArray(cp,n,0,longestLeg);
    cp->post(Factory::circuit(succ,gpu ? GPU : CPU));
    for(int i=0;i < n;i++)
        cp->post(Factory::element(distanceMatrix[i],succ[i],distSucc[i]));

    Objective::Ptr obj = Factory::minimize(Factory::sum(distSucc));
       
    DFSearch search(cp,firstFail(cp,succ));
    std::vector<int> best(n);

    search.onSolution([&succ,&best,&obj]() {
                          for(auto i=0u;i < best.size();i++)
                              best[i] = succ[i]->min();
                          cout << "objective = " << obj->value() << endl;
                      });

    auto limits = [=](SearchStatistics const & ss) {
        return RuntimeMonitor::elapsedSeconds(ss.getStartTime()) > timeout;
    };
    auto stat = search.optimize(obj,limits);
    stat.setPropagations(cp->getPropagations());
    cout << stat << endl;

    cout << "tour =";
    for (auto i = 0, city = 0; i < n; i += 1, city = best[city])
        cout << " " << city;
    cout << endl;

    cp.dealloc();
    return 0;
}
