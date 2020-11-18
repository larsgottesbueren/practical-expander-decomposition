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

DEFINE_double(
    phi, 0.01,
    "Value of \\phi such that expansion of each cluster is at least \\phi");
DEFINE_int32(t1, 100, "Constant 't1' in 'T = t1 + t2 \\log^2 m'");
DEFINE_double(t2, 1.0, "Constant 't2' in 'T = t1 + t2 \\log^2 m'");
DEFINE_bool(one_indexed, false, "Vertices are 1-indexed instead of 0-indexed.");

int main(int argc, char *argv[]) {
  google::InitGoogleLogging(argv[0]);
  gflags::ParseCommandLineFlags(&argc, &argv, true);

  int n, m;
  cin >> n >> m;
  auto g = make_unique<Undirected::Graph>(n);
  for (int i = 0; i < m; ++i) {
    int u, v;
    cin >> u >> v;
    if (FLAGS_one_indexed)
      u--, v--;
    if (u < v)
      g->addEdge(u, v);
  }

  ExpanderDecomposition::Solver solver(move(g), FLAGS_phi, FLAGS_t1, FLAGS_t2);
  auto partitions = solver.getPartition();

  cout << solver.getEdgesCut() << " " << partitions.size() << endl;
  for (const auto &p : partitions) {
    cout << p.size();
    for (const int u : p)
      cout << " " << u;
    cout << endl;
  }
}