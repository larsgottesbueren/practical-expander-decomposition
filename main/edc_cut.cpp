#include <chrono>
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

DEFINE_uint32(seed, 0, "Seed randomness with any positive integer. Default value '0' means a random seed will be chosen based on system time.");
DEFINE_double(
    phi, 0.01,
    "Value of \\phi such that expansion of each cluster is at least \\phi");
DEFINE_int32(t1, -1, "Constant 't1' in 'T = t1 + t2 \\log^2 m'. Will be chosen by strategy if not chosen manually.");
DEFINE_double(t2, -1.0, "Constant 't2' in 'T = t1 + t2 \\log^2 m'. Will be chosen by strategy if not chosen manually.");
DEFINE_int32(
    min_iterations, 0,
    "Minimum iterations to run cut-matching game. If this is larger than 'T' "
    "then certificate of expansion can be effected due to extra congestion.");
DEFINE_double(
    min_balance, 0.25,
    "The amount of cut balance before the cut-matching game is terminated.");
DEFINE_bool(chaco, false,
            "Input graph is given in the Chaco graph file format");
DEFINE_bool(sample_potential, false,
            "True if the potential function should be sampled.");
DEFINE_bool(balanced_cut_strategy, true,
            "Propose perfectly balanced cuts in the cut-matching game. This "
            "results in faster convergance of the potential function.");
DEFINE_bool(record_cut_matching_time, false,
            "Record time taken for cut-matching game to run excluding setup "
            "and post-processing of results.");

int main(int argc, char *argv[]) {
  google::InitGoogleLogging(argv[0]);

  gflags::SetUsageMessage("Expander Decomposition & Clustering");
  gflags::ParseCommandLineFlags(&argc, &argv, true);

  auto randomGen = configureRandomness(FLAGS_seed);

  VLOG(1) << "Reading input.";
  auto g = readGraph(FLAGS_chaco);
  VLOG(1) << "Finished reading input.";

  const int default_t1 = FLAGS_balanced_cut_strategy ? 22 : 124;
  const double default_t2 = FLAGS_balanced_cut_strategy ? 5.07 : 19.1;

  CutMatching::Parameters params = {.tConst = FLAGS_t1 < -0.5 ? default_t1 : FLAGS_t1,
                                    .tFactor = FLAGS_t2 < -0.5 ? default_t2 : FLAGS_t2,
                                    .minIterations = FLAGS_min_iterations,
                                    .minBalance = FLAGS_min_balance,
                                    .samplePotential = FLAGS_sample_potential,
                                    .balancedCutStrategy =
                                        FLAGS_balanced_cut_strategy};

  auto graph = ExpanderDecomposition::constructFlowGraph(g);
  auto subdivGraph = ExpanderDecomposition::constructSubdivisionFlowGraph(g);

  auto subdivisionIdx =
      std::make_unique<std::vector<int>>(subdivGraph->size(), -1);
  auto fromSubdivisionIdx =
      std::make_unique<std::vector<int>>(subdivGraph->size(), -1);
  for (int u = graph->size(); u < subdivGraph->size(); ++u)
    (*subdivisionIdx)[u] = 0;

  CutMatching::Solver solver(graph.get(), subdivGraph.get(), randomGen.get(),
                             subdivisionIdx.get(), fromSubdivisionIdx.get(),
                             FLAGS_phi, params);

  auto timeBefore = std::chrono::high_resolution_clock::now();
  auto result = solver.compute(params);
  auto timeAfter = std::chrono::high_resolution_clock::now();

  std::vector<int> a, r;
  std::copy(graph->cbegin(), graph->cend(), std::back_inserter(a));
  int edgesCut = 0;
  for (auto u : *graph)
    edgesCut += graph->globalDegree(u) - graph->degree(u);
  std::copy(graph->cbeginRemoved(), graph->cendRemoved(),
            std::back_inserter(r));
  graph->restoreRemoves();

  switch (result.type) {
  case CutMatching::Result::Balanced: {
    cout << "balanced_cut"
         << " " << result.iterations;
    break;
  }
  case CutMatching::Result::NearExpander: {
    cout << "near_expander"
         << " " << result.iterations;
    break;
  }
  case CutMatching::Result::Expander: {
    cout << "expander"
         << " " << result.iterationsUntilValidExpansion;
    break;
  }
  }
  cout << " " << graph->volume(a.begin(), a.end()) << " "
       << graph->volume(r.begin(), r.end()) << " " << edgesCut << endl;

  cout << a.size();
  for (auto u : a)
    cout << " " << u;
  cout << endl;
  cout << r.size();
  for (auto u : r)
    cout << " " << u;
  cout << endl;

  if (FLAGS_sample_potential) {
    cout << result.sampledPotentials.size() << endl;
    bool first = true;
    for (auto p : result.sampledPotentials) {
      if (!first)
        cout << " ";
      else
        first = false;
      cout << p;
    }
    cout << endl;
  }

  if (FLAGS_record_cut_matching_time) {
    using chrono::duration_cast;
    using chrono::milliseconds;
    cout << duration_cast<milliseconds>(timeAfter - timeBefore).count() << endl;
  }
}
