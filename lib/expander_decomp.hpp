#pragma once

#include <vector>

#include "cut_matching.hpp"
#include "partition_graph.hpp"

struct ExpanderDecomp {
private:
  std::unique_ptr<PartitionGraph<int, Edge>> graph;
  std::unique_ptr<UnitFlow> flowGraph, subdivisionFlowGraph;
  const double phi;

  /**
     Compute expander decomposition for subset of vertices 'xs'.
   */
  void compute(const std::vector<int> &xs, int partition);

public:
  /**
     Create a decomposition problem with n vertices.
   */
  ExpanderDecomp(const PartitionGraph<int, Edge> &g, const double phi);

  /**
     Return the computed partition as a vector of disjoint vertex vectors.
   */
  std::vector<std::vector<int>> getPartition() const;
};
