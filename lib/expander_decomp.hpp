#pragma once

#include <vector>

#include "cut_matching.hpp"
#include "graph.hpp"

struct ExpanderDecomp {
private:
  Graph graph;
  int partitions;

  void go(double phi, std::vector<Vertex> vs);

public:
  /**
     'partition[u]' is the partition index of vertex 'u'.
   */
  std::vector<int> partition;

  /**
     Create a decomposition problem with n vertices.
   */
  ExpanderDecomp(Graph g);

  std::vector<std::vector<Vertex>> compute(double phi) const;
};
