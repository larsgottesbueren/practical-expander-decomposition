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
DEFINE_int32(t1, 150, "Constant 't1' in 'T = t1 + t2 \\log^2 m'");
DEFINE_double(t2, 20.0, "Constant 't2' in 'T = t1 + t2 \\log^2 m'");
DEFINE_bool(resample_unit_vector, false,
            "Should random unit vector in cut-matching game be resampled every "
            "iteration. Results in slower execution time.");
DEFINE_int32(random_walk_steps, 10,
             "Number of random walk steps in cut-matching game if "
             "'resample_unit_vector=true'.");
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

  auto randomGen = configureRandomness();

  VLOG(1) << "Reading input.";
  auto g = readGraph(FLAGS_chaco);
  VLOG(1) << "Finished reading input.";

  CutMatching::Parameters params = {
      .tConst = FLAGS_t1,
      .tFactor = FLAGS_t2,
      .resampleUnitVector = FLAGS_resample_unit_vector,
      .minBalance = FLAGS_min_balance,
      .randomWalkSteps = FLAGS_random_walk_steps,
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
