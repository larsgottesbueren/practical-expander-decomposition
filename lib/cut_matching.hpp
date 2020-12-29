#pragma once

#include <random>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "datastructures/undirected_graph.hpp"
#include "datastructures/unit_flow.hpp"

namespace CutMatching {

enum ResultType { Balanced, Expander, NearExpander };

/**
   The result of running the cut-matching game is a balanced cut, an expander,
   or a near expander. If an expander or near expander is found, sample the
   expansion certificate 'verifyExpansion' number of times.
 */
struct Result {
  ResultType type;
  std::vector<double> certificateSamples;
};

using Matching = std::vector<std::pair<int, int>>;
std::vector<double> projectFlow(const std::vector<Matching> &rounds,
                                const std::vector<int> &fromSplitNode,
                                std::vector<double> start);

class Solver {
private:
  UnitFlow::Graph *graph;
  UnitFlow::Graph *subdivGraph;

  const double phi;
  const double T;
  const int verifyExpansion;

  std::mt19937 randomGen;

public:
  /**
     Create a cut-matching problem from a graph.

     Precondition: graph should not contain loops.
   */
  Solver(UnitFlow::Graph *g, UnitFlow::Graph *subdivGraph, const double phi,
         const int tConst, const double tFactor, const int verifyExpansion);

  Result compute();
};
}; // namespace CutMatching
