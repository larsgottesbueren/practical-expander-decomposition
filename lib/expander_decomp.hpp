#pragma once

#include <vector>

#include "cut_matching.hpp"
#include "graph.hpp"

struct ExpanderDecomp {
  Graph graph;

  /**
     Create a decomposition problem with n vertices.
   */
  ExpanderDecomp(int n);

  std::vector<std::vector<Vertex>> compute() const;
};
