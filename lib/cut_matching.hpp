#pragma once

#include <random>
#include <unordered_map>
#include <vector>

#include "partition_graph.hpp"
#include "unit_flow.hpp"

struct CutMatching {
private:
  const std::unique_ptr<PartitionGraph<int, Edge>> &graph;
  const std::vector<int> subset;

  /**
     Re-maps the vertices in 'subset' to the range '[0,|subset|)'.

     'fromSubset[subset[i]] = i'
   */
  std::unordered_map<int, int> fromSubset;

  const int graphPartition;
  const double phi;

  UnitFlow flowInstance;

  std::mt19937 randomGen;

public:
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

  /**
     Create a cut-matching problem from a graph.

     Precondition: graph should not contain loops.
   */
  CutMatching(const std::unique_ptr<PartitionGraph<int, Edge>> &g,
              const std::vector<int> &subset, const int graphPartition,
              const double phi);

  Result compute();
};
