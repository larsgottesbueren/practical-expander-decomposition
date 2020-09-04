#pragma once

#include <vector>

/**
   A graph which supports partitioning vertices into separate sub-graphs.

   Notation:
   - G=(V,E)
   - V = V_1 + V_2 + ... + V_k
   - E = E_1 + E_2 + ... + E_k
   - n = |V|, m = |E|, n_i = |V_i|, m_i = |E_i|

 */
struct PartitionGraph {
  using Vertex = int;

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
  std::vector<std::vector<Vertex>> graph;

  /**
     Subgraph where edges have been deleted if they belong to separate
     partitions.
   */
  std::vector<std::vector<Vertex>> pGraph;

public:
  /**
     Construct an empty graph with a single partition.
   */
  PartitionGraph(const int n);

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
  int volume(const std::vector<Vertex> &xs) const;

  /**
     The partition vertex 'u' is a part of.

     Time complexity: O(1)
   */
  int getPartition(const int u) const { return partition[u]; };

  /**
     Neighbors of 'u' in entire graph.

     Size of result: O(|E|)
     Time complexity: O(1)
   */
  const std::vector<Vertex> &neighbors(const int u) const { return graph[u]; };

  /**
     Neighbours of 'u' in its partition.

     Size of result: O(|E_i|), where u \in V_i.
     Time complexity: O(1)
   */
  const std::vector<Vertex> &partitionNeighbors(const int u) const {
    return pGraph[u];
  };

  /**
     Add an undirected edge '{u,v}'. If 'u = v' do nothing.

     Time complexity: O(1)
   */
  void addEdge(const Vertex u, const Vertex v);

  /**
     Create a new partition for the nodes 'xs' given the current nodes in the
     partition 'ys'. Return the index of the new partition.

     Precondition: All 'xs \subseteq ys' and all 'y \in ys' are in the same
     partition.

     Time complexity: O(|V_i| + |E_i|) where 'ys \subseteq V_i'
   */
  int newPartition(const std::vector<Vertex> &xs,
                   const std::vector<Vertex> &ys);
};
