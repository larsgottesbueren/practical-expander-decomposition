#pragma once

#include <vector>

using Vertex = int;

struct Graph {
private:
  int numEdges;

public:
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
     Number of edges currently in graph.
   */
  int edgeCount() const { return numEdges; }

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

  /**
     Volume of a list of vertices.

     Time complexity: O(|xs|)
   */
  template <typename It> int volume(It begin, It end) const {
    int vol = 0;
    for (It it = begin; it != end; ++it)
      vol += degree(*it);
    return vol;
  }
};
