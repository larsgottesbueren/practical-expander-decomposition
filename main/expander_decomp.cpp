#include <cmath>
#include <gflags/gflags.h>
#include <glog/logging.h>
#include <glog/stl_logging.h>
#include <iostream>
#include <numeric>
#include <vector>

#include "lib/cut_matching.hpp"
#include "lib/datastructures/undirected_graph.hpp"
#include "lib/expander_decomp.hpp"

using namespace std;

DEFINE_double(
    phi, 0.01,
    "Value of \\phi such that expansion of each cluster is at least \\phi");
DEFINE_int32(t1, 100, "Constant 't1' in 'T = t1 + t2 \\log^2 m'");
DEFINE_double(t2, 1.0, "Constant 't2' in 'T = t1 + t2 \\log^2 m'");
DEFINE_bool(chaco, false,
            "Input graph is given in the Chaco graph file format.");
DEFINE_bool(partitions, false, "Output indices of partitions.");

int main(int argc, char *argv[]) {
  google::InitGoogleLogging(argv[0]);
  gflags::ParseCommandLineFlags(&argc, &argv, true);

  int n, m;
  cin >> n >> m;

  vector<Undirected::Edge> es;
  if (FLAGS_chaco) {
    cin.ignore();
    for (int u = 0; u < n; ++u) {
      string line;
      cin >> line;
      stringstream ss(line);
      int v;
      while (ss >> v)
        es.push_back({u, v - 1});
    }
  } else {
    for (int i = 0; i < m; ++i) {
      int u, v;
      cin >> u >> v;
      if (u < v)
        es.push_back({u, v - 1});
    }
  }

  ExpanderDecomposition::Solver solver(make_unique<Undirected::Graph>(n, es),
                                       FLAGS_phi, FLAGS_t1, FLAGS_t2);
  auto partitions = solver.getPartition();
  auto conductances = solver.getConductance();

  cout << solver.getEdgesCut() << " " << partitions.size() << endl;
  for (int i = 0; i < int(partitions.size()); ++i) {
    cout << partitions[i].size() << " " << conductances[i];
    if (FLAGS_partitions)
      for (auto p : partitions[i])
        cout << " " << p;
    cout << endl;
  }
}
