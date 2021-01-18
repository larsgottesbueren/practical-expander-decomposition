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
#include "util.hpp"

using namespace std;

DEFINE_double(
    phi, 0.01,
    "Value of \\phi such that expansion of each cluster is at least \\phi");
DEFINE_int32(t1, 40, "Constant 't1' in 'T = t1 + t2 \\log^2 m'");
DEFINE_double(t2, 2.2, "Constant 't2' in 'T = t1 + t2 \\log^2 m'");
DEFINE_int32(random_walk_steps, 10,
             "Number of random walk steps in cut-matching game.");
DEFINE_bool(chaco, false,
            "Input graph is given in the Chaco graph file format");
DEFINE_bool(partitions, false, "Output indices of partitions");
DEFINE_int32(verify_expansion, 0,
             "Number of times an expansion certificate generated by the "
             "cut-matching game should be sampled");

int main(int argc, char *argv[]) {
  google::InitGoogleLogging(argv[0]);

  gflags::SetUsageMessage("Expander Decomposition & Clustering");
  gflags::ParseCommandLineFlags(&argc, &argv, true);

  VLOG(1) << "Reading input.";
  auto g = readGraph(FLAGS_chaco);
  VLOG(1) << "Finished reading input.";

  ExpanderDecomposition::Solver solver(move(g), FLAGS_phi, FLAGS_t1, FLAGS_t2,
                                       FLAGS_random_walk_steps,
                                       FLAGS_verify_expansion);
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
