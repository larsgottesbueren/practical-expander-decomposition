#include "graph.hpp"

Graph::Graph(int n) : numEdges(0), neighbors(n) {}

Graph::Graph(const Graph &g) : numEdges(g.numEdges), neighbors(g.neighbors) {}

void Graph::addEdge(Vertex u, Vertex v) {
  neighbors[u].push_back(v);
  neighbors[v].push_back(u);
  numEdges++;
}
