#pragma once

#include <vector>

#include "cut_matching.hpp"
#include "partition_graph.hpp"

struct ExpanderDecomp {
private:
  PartitionGraph graph;
  const double phi;

  /**
     Compute expander decomposition for subset of vertices 'xs'.
   */
  void compute(const std::vector<PartitionGraph::Vertex> &xs, int partition);

public:
  /**
     Create a decomposition problem with n vertices.
   */
  ExpanderDecomp(PartitionGraph &g, const double phi);

  /**
     Return the computed partition as a vector of disjoint vertex vectors.
   */
  std::vector<std::vector<PartitionGraph::Vertex>> getPartition() const;
};
