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

  ExpanderDecomposition::Solver solver(move(g), phi, tConst, tFactor);
  auto partitions = solver.getPartition();

  cout << solver.getEdgesCut() << " " << partitions.size() << endl;
  for (const auto &p : partitions) {
    cout << p.size();
    for (const int u : p)
      cout << " " << u;
    cout << endl;
  }
}
