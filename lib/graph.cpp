#include "graph.hpp"

Graph::Graph(int n) : neighbors(n) {}

void Graph::addEdge(Vertex u, Vertex v) {
  neighbors[u].push_back(v);
  neighbors[v].push_back(u);
}

int Graph::volume(std::vector<Vertex> xs) const {
  int vol = 0;
  for (auto x : xs)
    vol += degree(x);
  return vol;
}
