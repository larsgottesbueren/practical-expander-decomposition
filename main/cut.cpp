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

  vector<int> xs(n);
  iota(xs.begin(), xs.end(), 0);
  auto flowGraph = ExpanderDecomposition::constructFlowGraph(g);
  auto subdivisionFlowGraph =
      ExpanderDecomposition::constructSubdivisionFlowGraph(g);

  auto solver = CutMatching::Solver(flowGraph.get(), subdivisionFlowGraph.get(),
                                    xs, FLAGS_phi, FLAGS_t1, FLAGS_t2);
  auto result = solver.compute();
  flowGraph->newPartition(result.a, xs);

  cout << flowGraph->volume(result.a.begin(), result.a.end()) << " "
       << flowGraph->volume(result.r.begin(), result.r.end()) << endl
       << result.a.size() << endl;
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
