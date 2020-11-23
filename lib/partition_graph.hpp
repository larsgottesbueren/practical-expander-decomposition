#pragma once

#include <queue>
#include <vector>

#include "absl/container/flat_hash_set.h"

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
template <typename V, typename E> class PartitionGraph {

private:
  int numPartitions;
  const int numVertices;
  int numEdges;
  std::vector<int> originalDegree;
  std::vector<int> numEdgesInPartition;
  std::vector<int> numVerticesInPartition;

  /**
     Used to mark vertex as visited in search algorithms. Assume it can contain
     arbitrary values; reset relevant vertices before use.
   */
  std::vector<bool> visited;

  /**
     'partition[u] = p' indicates vertex 'u' is in partition 'p'.
   */
  std::vector<int> partition;

  /**
     Graph where only valid edges are kept. An edge is valid if it does not
     cross a partition boundary.
   */
  std::vector<std::vector<std::unique_ptr<E>>> graph;

protected:
  /**
     Add directed edge '(u,v)'. If 'u = v' do nothing. If vertices are in
     separate partitions, edge is not added but global degree of 'u' is
     incremented.

     Returns a pointer to the edge which was added or null otherwise.

     Time complexity: O(1)
   */
  E *addDirectedEdge(E e, bool incrementEdgeCount) {
    V u = e.from, v = e.to;

    assert(u < numVertices && v < numVertices && "Vertex out of bounds.");
    if (u == v)
      return nullptr;

    originalDegree[u]++;
    if (partition[u] != partition[v]) {
      return nullptr;
    } else {
      if (incrementEdgeCount)
        numEdges++, numEdgesInPartition[partition[u]]++;
      graph[u].emplace_back(std::make_unique<E>(e));

      return graph[u].back().get();
    }
  }

public:
  /**
     Construct an empty graph with a single partition.
   */
  PartitionGraph(const int n)
      : numPartitions(1), numVertices(n), numEdges(0), originalDegree(n),
        numEdgesInPartition(1, 0), numVerticesInPartition(1, n), visited(n),
        partition(n), graph(n) {}

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
     Degree of vertex 'u' in sub-graph.

     Time complexity: O(1)
   */
  int degree(const int u) const { return graph[u].size(); };

  /**
     Degree of vertex 'u' in entire graph.

     Time complexity: O(1)
   */
  int globalDegree(const int u) const { return originalDegree[u]; };

  /**
     The volume of a subset of nodes in their subgraph.

     Time complexity: O(|xs|)
   */
  template <typename It> int volume(const It &begin, const It &end) const {
    int result = 0;
    for (It it = begin; it != end; ++it)
      result += degree(*it);
    return result;
  };

  /**
     The volume of a subset of nodes in the entire graph.

     Time complexity: O(|xs|)
   */
  template <typename It>
  int globalVolume(const It &begin, const It &end) const {
    int result = 0;
    for (It it = begin; it != end; ++it)
      result += globalDegree(*it);
    return result;
  };

  /**
     The partition vertex 'u' is a part of.

     Time complexity: O(1)
   */
  int getPartition(const int u) const { return partition[u]; };

  /**
     Edges from 'u' in sub-graph.

     Size of result: O(|E_i|), where u \in V_i.
     Time complexity: O(1)
   */
  std::vector<std::unique_ptr<E>> &edges(const int u) { return graph[u]; };
  const std::vector<std::unique_ptr<E>> &edges(const int u) const {
    return graph[u];
  };

  /**
     Edges from 'u' in its partition.

     Time complexity: O(|E_i|), where u \in V_i.
   */
  std::vector<V> neighbors(const int u) const {
    std::vector<V> vs(graph[u].size());
    for (int i = 0; i < (int)graph[u].size(); ++i)
      vs[i] = graph[u][i]->to;

    return vs;
  };

  /**
     Find all connected components of a subset of vertices. Return a vector of
     these components.
   */
  std::vector<std::vector<V>> connectedComponents(const std::vector<V> &xs) {
    for (auto u : xs)
      visited[u] = false;

    std::vector<std::vector<V>> comps;

    auto search = [&](V start) {
      std::queue<V> q;
      visited[start] = true;
      q.push(start);
      while (!q.empty()) {
        int u = q.front();
        q.pop();
        comps.back().push_back(u);
        for (const auto &e : edges(u))
          if (!visited[e->to])
            visited[e->to] = true, q.push(e->to);
      }
    };

    for (auto u : xs)
      if (!visited[u]) {
        comps.push_back({});
        search(u);
      }

    return comps;
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
      graph[u].erase(std::remove_if(graph[u].begin(), graph[u].end(),
                                    [&newP, this](const auto &e) {
                                      return partition[e->to] != newP;
                                    }),
                     graph[u].end());
      numEdgesInPartition[newP] += graph[u].size();
    }
    numEdgesInPartition[newP] /= 2; // Edges are double counted above

    numEdgesInPartition[oldP] = 0;
    for (const auto u : ys)
      if (partition[u] == oldP) {
        graph[u].erase(std::remove_if(graph[u].begin(), graph[u].end(),
                                      [&oldP, this](const auto &e) {
                                        return partition[e->to] != oldP;
                                      }),
                       graph[u].end());
        numEdgesInPartition[oldP] += graph[u].size();
      }
    numEdgesInPartition[oldP] /= 2; // Edges are double counted above

    return newP;
  }
};
