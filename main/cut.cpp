#include <cmath>
#include <gflags/gflags.h>
#include <glog/logging.h>
#include <glog/stl_logging.h>
#include <iostream>
#include <numeric>
#include <vector>

#include "lib/cut_matching.hpp"
#include "lib/expander_decomp.hpp"
#include "lib/ugraph.hpp"

using namespace std;

int main(int argc, char *argv[]) {
  google::InitGoogleLogging(argv[0]);
  gflags::ParseCommandLineFlags(&argc, &argv, true);

  double phi;
  int tConst;
  double tFactor;
  int n, m;
  cin >> phi >> tConst >> tFactor >> n >> m;
  auto g = make_unique<Undirected::Graph>(n);
  for (int i = 0; i < m; ++i) {
    int u, v;
    cin >> u >> v;
    g->addEdge(u, v);
  }

  vector<int> xs(n);
  iota(xs.begin(), xs.end(), 0);
  auto flowGraph = ExpanderDecomposition::constructFlowGraph(g);
  auto subdivisionFlowGraph =
      ExpanderDecomposition::constructSubdivisionFlowGraph(g);

  auto solver = CutMatching::Solver(flowGraph.get(), subdivisionFlowGraph.get(),
                                    xs, phi, tConst, tFactor);
  auto result = solver.compute();

  cout << result.a.size() << endl;
  for (int i = 0; i < (int)result.a.size(); ++i) {
    if (i != 0)
      cout << " ";
    cout << result.a[i];
  }

  cout << endl << result.r.size() << endl;
  for (int i = 0; i < (int)result.r.size(); ++i) {
    if (i != 0)
      cout << " ";
    cout << result.r[i];
  }
  cout << endl;
}
