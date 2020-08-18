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
  enum ResultType { Balanced, Expander, NearExpander };
  /**
     The cut-matching algorithm can return one of three types of result.
     - Balanced: (a,r) is a balanced cut
     - Expander: a is a phi expander
     - NearExpander: a is a nearly phi expander
   */
  struct Result {
    ResultType t;
    std::vector<Vertex> a, r;
  };

  const int numRegularNodes;
  /**
     Number of "split nodes", i.e. those which were created from edges.
   */
  const int numSplitNodes;

  /**
     Create a cut-matching problem from a graph.

     Precondition: graph should not contain loops.
   */
  CutMatching(const Graph &g);

  Result compute(double phi) const;
};
