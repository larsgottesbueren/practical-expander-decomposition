#pragma once

#include <random>
#include <vector>
#include <unordered_map>

#include "partition_graph.hpp"
#include "unit_flow.hpp"

struct CutMatching {
private:
  const PartitionGraph &graph;
  const std::vector<PartitionGraph::Vertex> &subset;
  /**
     Re-maps the vertices in 'subset' to the range '[0,|subset|)'.

     'fromSubset[subset[i]] = i'
   */
  std::unordered_map<int,int> fromSubset;

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
    std::vector<PartitionGraph::Vertex> a, r;
  };

  /**
     Create a cut-matching problem from a graph.

     Precondition: graph should not contain loops.
   */
  CutMatching(const PartitionGraph &g,
              const std::vector<PartitionGraph::Vertex> &subset,
              const int graphPartition, const double phi);

  Result compute();
};
