#include "partition_graph.hpp"
#include <algorithm>

PartitionGraph::PartitionGraph(const int n)
    : numPartitions(1), numVertices(n), numEdges(0), numEdgesInPartition(1, 0),
      numVerticesInPartition(1, n), partition(n), graph(n), pGraph(n) {}

void PartitionGraph::addEdge(const Vertex u, const Vertex v) {
  assert(u < numVertices && v < numVertices && "Vertex out of bounds.");
  if (u == v)
    return;

  numEdges++;

  graph[u].push_back(v);
  graph[v].push_back(u);
  if (partition[u] == partition[v]) {
    numEdgesInPartition[partition[u]]++;

    pGraph[u].push_back(v);
    pGraph[v].push_back(u);
  }
}

void PartitionGraph::newPartition(const std::vector<Vertex> &xs,
                                  const std::vector<Vertex> &ys) {
  const int oldP = partition[xs[0]];
  const int newP = numPartitions++;
  for (const auto u : xs)
    partition[u] = newP;

  numEdgesInPartition.push_back(0);
  numVerticesInPartition.push_back((int)xs.size());
  numVerticesInPartition[oldP] -= numVerticesInPartition[newP];

  for (const auto u : xs) {
    pGraph[u].erase(std::remove_if(pGraph[u].begin(), pGraph[u].end(),
                                   [&newP, this](const int v) {
                                     return partition[v] != newP;
                                   }),
                    pGraph[u].end());
    numEdgesInPartition[newP] += pGraph[u].size();
  }
  numEdgesInPartition[newP] /= 2; // Edges are double counted above

  numEdgesInPartition[oldP] = 0;
  for (const auto u : ys)
    if (partition[u] == oldP) {
      pGraph[u].erase(std::remove_if(pGraph[u].begin(), pGraph[u].end(),
                                     [&oldP, this](const int v) {
                                       return partition[v] != oldP;
                                     }),
                      pGraph[u].end());
      numEdgesInPartition[oldP] += pGraph[u].size();
    }
  numEdgesInPartition[oldP] /= 2; // Edges are double counted above
}
