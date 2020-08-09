#pragma once

#include <vector>

#include "graph.hpp"

struct CutMatching {
private:
  /**
     A sub-division graph. See definition B.1 Saranurak et al. Given graph
     G=(V,E), subdivision vertices start at index |V|.
   */
  Graph graph;

public:
  const int numRegularNodes;

  /**
     Number of "split nodes", i.e. those which were created from edges.
   */
  int numSplitNodes() const { return graph.size() - numRegularNodes; }

  /**
     Create a cut-matching problem from a graph.

     Precondition: graph should not contain loops.
   */
  CutMatching(const Graph &g);

  std::pair<std::vector<Vertex>, std::vector<Vertex>> compute(double phi) const;
};
