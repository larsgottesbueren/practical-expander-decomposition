#pragma once

#include <vector>

using Vertex = int;

struct Graph {
  std::vector<std::vector<Vertex>> neighbors;

  /**
     Construct a graph with n vertices.

     Time complexity: O(n)
   */
  Graph(int n);

  /**
     Number of vertices in graph.

     Time complexity: O(1)
   */
  int size() const { return (int)neighbors.size(); }

  /**
     Degree of vertex.

     Time complexity: O(1)
   */
  int degree(Vertex u) const { return (int)neighbors[u].size(); }

  /**
     Add an edge between two vertices.

     Time complexity: O(1)
   */
  void addEdge(Vertex u, Vertex v);
};
