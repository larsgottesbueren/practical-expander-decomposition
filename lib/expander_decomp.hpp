#pragma once

#include <vector>

#include "cut_matching.hpp"
#include "partition_graph.hpp"

struct ExpanderDecomp {
private:
  /**
     One graph and two flow graphs are maintained. The vertices and edges in
     'graph' and 'flowGraph' are identical apart from the edges maintaining flow
     information in the latter. Let 'graph = (V,E)'. Then '{e.id + |V| | e \in
     E}' is the vertex ids of the split vertices in 'subdivisionFlowGraph'.
   */
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
