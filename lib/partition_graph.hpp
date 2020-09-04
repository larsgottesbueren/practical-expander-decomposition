#pragma once

#include <vector>

/**
   A simple directed edge.
 */
struct Edge {
  int from, to;
  Edge(int from, int to) : from(from), to(to) {}
  Edge rev() const {
    Edge e(to, from);
    return e;
  }
};

/**
   A graph which supports partitioning vertices into separate sub-graphs.

   Parameterised over vertex type <V> and edge type <E>. An edge must have
   members '.from' and '.to' with type <V> as well 'Edge rev() const'.


   Notation:
   - G=(V,E)
   - V = V_1 + V_2 + ... + V_k
   - E = E_1 + E_2 + ... + E_k
   - n = |V|, m = |E|, n_i = |V_i|, m_i = |E_i|
 */
template <typename V, typename E> struct PartitionGraph {

private:
  int numPartitions;
  const int numVertices;
  int numEdges;
  std::vector<int> numEdgesInPartition;
  std::vector<int> numVerticesInPartition;

  /**
     'partition[u] = p' indicates vertex 'u' is in partition 'p'.
   */
  std::vector<int> partition;

  /**
     Entire graph as a neighbor list.
   */
  std::vector<std::vector<E>> graph;

  /**
     Subgraph where edges have been deleted if they belong to separate
     partitions.
   */
  std::vector<std::vector<E>> pGraph;

public:
  /**
     Construct an empty graph with a single partition.
   */
  PartitionGraph(const int n)
      : numPartitions(1), numVertices(n), numEdges(0),
        numEdgesInPartition(1, 0), numVerticesInPartition(1, n), partition(n),
        graph(n), pGraph(n) {}

  /**
     Number of vertices in entire graph.

     Time complexity: O(1)
   */
  int size() const { return numVertices; }

  /**
     Number of vertices in partition 'i'.

     Time complexity: O(1)
   */
  int size(const int i) const { return numVerticesInPartition[i]; }

  /**
     Number of edges in entire graph.

     Time complexity: O(1)
   */
  int edgeCount() const { return numEdges; }

  /**
     Number of edges in partition 'i'.

     Time complexity: O(1)
   */
  int edgeCount(const int i) const { return numEdgesInPartition[i]; }

  /**
     The number of partitions.

     Time complexity: O(1)
   */
  int partitionCount() const { return numPartitions; }

  /**
     Degree of vertex 'u' in entire graph.

     Time complexity: O(1)
   */
  int degree(const int u) const { return graph[u].size(); };

  /**
     Degree of vertex 'u' in its partition.

     Time complexity: O(1)
   */
  int partitionDegree(const int u) const { return pGraph[u].size(); };

  /**
     The volume of a subset of nodes is the sum of their degrees.

     Time complexity: O(|xs|)
   */
  template <typename It> int volume(const It &begin, const It &end) const {
    int result = 0;
    for (It it = begin; it != end; ++it)
      result += degree(*it);
    return result;
  };

  /**
     The volume of a subset of nodes in their partition.

     Time complexity: O(|xs|)
   */
  template <typename It>
  int partitionVolume(const It &begin, const It &end) const {
    int result = 0;
    for (It it = begin; it != end; ++it)
      result += partitionDegree(*it);
    return result;
  };

  /**
     The partition vertex 'u' is a part of.

     Time complexity: O(1)
   */
  int getPartition(const int u) const { return partition[u]; };

  /**
     Edges from 'u' in entire graph.

     Size of result: O(|E|)
     Time complexity: O(1)
   */
  std::vector<E> &edges(const int u) { return graph[u]; };
  const std::vector<E> &edges(const int u) const { return graph[u]; };

  /**
     Edges from 'u' in its partition.

     Size of result: O(|E_i|), where u \in V_i.
     Time complexity: O(1)
   */
  std::vector<E> &partitionEdges(const int u) { return pGraph[u]; };
  const std::vector<E> &partitionEdges(const int u) const { return pGraph[u]; };

  /**
     Edges from 'u' in entire graph.

     Size of result: O(|E|)
     Time complexity: O(1)
   */
  std::vector<V> neighbors(const int u) const {
    std::vector<V> vs(graph[u].size());
    for (int i = 0; i < (int)graph[u].size(); ++i)
      vs[i] = graph[u].to;

    return vs;
  };

  /**
     Edges from 'u' in its partition.

     Size of result: O(|E_i|), where u \in V_i.
     Time complexity: O(1)
   */
  std::vector<V> partitionNeighbors(const int u) const {
    std::vector<V> vs(pGraph[u].size());
    for (int i = 0; i < (int)pGraph[u].size(); ++i)
      vs[i] = pGraph[u][i].to;

    return vs;
  };

  /**
     Add directed edge '{u,v}'. If 'u = v' do nothing.

     Time complexity: O(1)
   */
  void addEdge(const E &e) {
    V u = e.from, v = e.to;

    assert(u < numVertices && v < numVertices && "Vertex out of bounds.");
    if (u == v)
      return;

    numEdges++;
    graph[u].push_back(e);
    if (partition[u] == partition[v]) {
      numEdgesInPartition[partition[u]]++;

      pGraph[u].push_back(e);
    }
  }

  /**
     Add undirected edge '{u,v}'. If 'u = v' do nothing.

     Time complexity: O(1)
   */
  void addUEdge(const E &e) {
    V u = e.from, v = e.to;

    assert(u < numVertices && v < numVertices && "Vertex out of bounds.");
    if (u == v)
      return;

    numEdges++;
    graph[u].push_back(e);
    graph[v].push_back(e.rev());

    if (partition[u] == partition[v]) {
      numEdgesInPartition[partition[u]]++;

      pGraph[u].push_back(e);
      pGraph[v].push_back(e.rev());
    }
  }

  /**
     Create a new partition for the nodes 'xs' given the current nodes in the
     partition 'ys'. Return the index of the new partition.

     Precondition: All 'xs \subseteq ys' and all 'y \in ys' are in the same
     partition.

     Time complexity: O(|V_i| + |E_i|) where 'ys \subseteq V_i'
   */
  int newPartition(const std::vector<V> &xs, const std::vector<V> &ys) {
    const int oldP = partition[xs[0]];
    const int newP = numPartitions++;
    for (const auto u : xs)
      partition[u] = newP;

    numEdgesInPartition.push_back(0);
    numVerticesInPartition.push_back((int)xs.size());
    numVerticesInPartition[oldP] -= numVerticesInPartition[newP];

    for (const auto u : xs) {
      pGraph[u].erase(std::remove_if(pGraph[u].begin(), pGraph[u].end(),
                                     [&newP, this](const auto &e) {
                                       return partition[e.to] != newP;
                                     }),
                      pGraph[u].end());
      numEdgesInPartition[newP] += pGraph[u].size();
    }
    numEdgesInPartition[newP] /= 2; // Edges are double counted above

    numEdgesInPartition[oldP] = 0;
    for (const auto u : ys)
      if (partition[u] == oldP) {
        pGraph[u].erase(std::remove_if(pGraph[u].begin(), pGraph[u].end(),
                                       [&oldP, this](const auto &e) {
                                         return partition[e.to] != oldP;
                                       }),
                        pGraph[u].end());
        numEdgesInPartition[oldP] += pGraph[u].size();
      }
    numEdgesInPartition[oldP] /= 2; // Edges are double counted above

    return newP;
  }
};
