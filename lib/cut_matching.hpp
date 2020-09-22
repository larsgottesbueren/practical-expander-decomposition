#pragma once

#include <random>
#include <unordered_map>
#include <vector>

#include "ugraph.hpp"
#include "unit_flow.hpp"

namespace CutMatching {

enum ResultType { Balanced, Expander, NearExpander };
/**
   The cut-matching algorithm can return one of three types of result.
   - Balanced: (a,r) is a balanced cut
   - Expander: a is a phi expander
   - NearExpander: a is a nearly phi expander
 */
struct Result {
  ResultType t;
  std::vector<int> a, r;
};

using Matching = std::vector<std::pair<int, int>>;
std::vector<double> projectFlow(const std::vector<Matching> &rounds,
                                const std::unordered_map<int,int> &fromSplitNode,
                                std::vector<double> start);

class Solver {
private:
  const Undirected::Graph *graph;
  UnitFlow::Graph *subdivisionFlowGraph;
  const std::vector<int> subset;

  const int graphPartition;
  const double phi;

  std::mt19937 randomGen;

public:
  /**
     Create a cut-matching problem from a graph.

     Precondition: graph should not contain loops.
   */
  Solver(const Undirected::Graph *g, UnitFlow::Graph *subdivisionFlowGraph,
         const std::vector<int> &subset, const int graphPartition,
         const double phi);

  Result compute();
};
}; // namespace CutMatching
