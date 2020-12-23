
#pragma once

#include "subset_graph.hpp"

namespace Undirected {

/**
   Undirected edge with index to reverse edge.
 */
struct Edge {
  int from, to, revIdx;

  /**
     Construct an edge 'from->to'. 'revIdx' remains undefined.
   */
  Edge(int from, int to) : from(from), to(to), revIdx(-1) {}

  /**
     Construct the reverse of this edge. 'revIdx' remains undefined since it is
     maintained by the graph representation.
   */
  Edge reverse() const {
    Edge e{to, from};
    return e;
  }
};

/**
   An undirected graph is represented by subset graph.
 */
using Graph = SubsetGraph::Graph<int, Edge>;
} // namespace Undirected
