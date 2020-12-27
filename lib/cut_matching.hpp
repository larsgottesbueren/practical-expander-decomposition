#pragma once

#include <random>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "datastructures/undirected_graph.hpp"
#include "datastructures/unit_flow.hpp"

namespace CutMatching {

enum ResultType { Balanced, Expander, NearExpander };

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

  std::mt19937 randomGen;

public:
  /**
     Create a cut-matching problem from a graph.

     Precondition: graph should not contain loops.
   */
  Solver(UnitFlow::Graph *g, UnitFlow::Graph *subdivGraph, const double phi,
         const int tConst, const double tFactor);

  ResultType compute();
};
}; // namespace CutMatching
