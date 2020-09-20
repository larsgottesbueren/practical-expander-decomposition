#pragma once

#include "partition_graph.hpp"

namespace Undirected {
/**
   A directed edge with a pointer to its reverse.
*/
struct Edge {
  int from, to;
  Edge *reverse;
  Edge(int from, int to);
  Edge rev() const { return *reverse; };
};

/**
   An undirected graph.
 */
struct Graph : public PartitionGraph<int, Edge> {
  Graph(int n);

  /**
     Add undirected edge '{u,v}'. If 'u = v' do nothing. If vertices are in
     separate partitions, edge is not added but global degree of 'u' is
     incremented.

     Return true if an edge was added or false otherwise.

     Time complexity: O(1)
   */
  bool addEdge(int from, int to);
};
} // namespace Undirected
