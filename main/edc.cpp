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

DEFINE_uint32(seed, 0,
              "Seed randomness with any positive integer. Default value '0' "
              "means a random seed will be chosen based on system time.");
DEFINE_double(
    phi, 0.01,
    "Value of \\phi such that expansion of each cluster is at least \\phi");
DEFINE_int32(t1, -1,
             "Constant 't1' in 'T = t1 + t2 \\log^2 m'. Will be chosen by "
             "strategy if not chosen manually.");
DEFINE_double(t2, -1.0,
              "Constant 't2' in 'T = t1 + t2 \\log^2 m'. Will be chosen by "
              "strategy if not chosen manually.");
DEFINE_int32(
    min_iterations, 0,
    "Minimum iterations to run cut-matching game. If this is larger than 'T' "
    "then certificate of expansion can be effected due to extra congestion.");
DEFINE_double(
    min_balance, 0.45,
    "The amount of cut balance before the cut-matching game is terminated.");
DEFINE_bool(chaco, false,
            "Input graph is given in the Chaco graph file format");
DEFINE_bool(partitions, false, "Output indices of partitions");
DEFINE_bool(sample_potential, false,
            "True if the potential function should be sampled.");
DEFINE_bool(balanced_cut_strategy, true,
            "Propose perfectly balanced cuts in the cut-matching game. This "
            "results in faster convergance of the potential function.");

int main(int argc, char *argv[]) {
  google::InitGoogleLogging(argv[0]);

  gflags::SetUsageMessage("Expander Decomposition & Clustering");
  gflags::ParseCommandLineFlags(&argc, &argv, true);

  auto randomGen = configureRandomness(FLAGS_seed);

  VLOG(1) << "Reading input.";
  auto g = readGraph(FLAGS_chaco);
  VLOG(1) << "Finished reading input.";

  const int default_t1 = FLAGS_balanced_cut_strategy ? 22 : 142;
  const double default_t2 = FLAGS_balanced_cut_strategy ? 5.0 : 17.2;

  CutMatching::Parameters params = {
      .tConst = FLAGS_t1 < -0.5 ? default_t1 : FLAGS_t1,
      .tFactor = FLAGS_t2 < -0.5 ? default_t2 : FLAGS_t2,
      .minIterations = FLAGS_min_iterations,
      .minBalance = FLAGS_min_balance,
      .samplePotential = FLAGS_sample_potential,
      .balancedCutStrategy = FLAGS_balanced_cut_strategy};

  ExpanderDecomposition::Solver solver(move(g), FLAGS_phi, randomGen.get(),
                                       params);
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
