#pragma once

#include <algorithm>
#include <list>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "partition_graph.hpp"

/**
   Push relabel based unit flow algorithm. Based on push relabel in KACTL.
 */

struct UnitFlow {
public:
  using Vertex = int;
  using Flow = long long;

  struct Edge {
    const Vertex from, to;
    /**
        Index such that graph[to][backIdx] = edge to->from
    */
    const int backIdx;
    Flow flow, capacity;

    Edge(const Vertex from, const Vertex to, const int backIdx, Flow flow,
         Flow capacity);

    /**
       Reverse 'from' and 'to'.
     */
    Edge rev() const;
  };

private:
  /**
     For each vertex maintain a neighbor list.
   */
  PartitionGraph<int, Edge> graph;
  /**
     The amount of flow a vertex is absorbing. In the beginning, before any flow
     has been moved, this corresponds to the source function '\Delta(v)'.
   */
  std::vector<Flow> absorbed;
  /**
     The sink capacity of a vertex, i.e. the amount of flow possible to absorb.
   */
  std::vector<Flow> sink;
  /**
     The height of a vertex.
   */
  std::vector<Vertex> height;

  /**
     For each vertex, keep track of which edge in their neighbor list they
     should consider next.
   */
  std::vector<int> nextEdgeIdx;

  /**
     Residual capacity of an edge.
   */
  Flow residual(const Edge &e) const { return e.capacity - e.flow; }

public:
  /**
     Construct a unit flow problem with n vertices and maximum label height h.
   */
  UnitFlow(int n);

  const PartitionGraph<int, Edge> &getGraph() const { return graph; }
  const std::vector<Flow> &getAbsorbed() const { return absorbed; }
  const std::vector<Flow> &getSink() const { return sink; }
  const std::vector<Vertex> &getHeight() const { return height; }
  const std::vector<int> &getNextEdgeIdx() const { return nextEdgeIdx; }

  /**
     Add an undirected edge between two vertices with a certain capacity.

     If the edge forms a loop it is ignored.
   */
  void addEdge(Vertex u, Vertex v, Flow capacity);

  /**
     Increase the amount of flow a vertex is currently absorbing.
   */
  void addSource(Vertex u, Flow amount) { absorbed[u] += amount; }

  /**
     Increase the amount of flow a vertex is able to absorb on its own.
   */
  void addSink(Vertex u, Flow amount) { sink[u] += amount; }

  /**
      The amount of flow absorbed by a vertex.
  */
  Flow flowIn(Vertex u) const { return absorbed[u]; }

  /**
     The amount of flow leaving vertex.

     Time complexity: O(m)

     TODO: make this function O(1)
   */
  Flow flowOut(Vertex u) const {
    Flow f = 0;
    for (const auto &e : graph.edges(u))
      if (e.flow > 0)
        f += e.flow;
    return f;
  }

  /**
     Return the excess of a node, i.e. the flow it cannot absorb.
   */
  Flow excess(Vertex u) const {
    return std::max((Flow)0, absorbed[u] - sink[u]);
  }

  /**
     Compute max flow with push relabel and max height h. Return those vertices
     part of a level cut (height[u] >= h). If an empty vector is returned then
     all flow was possible to route.

     Extra log factor compared to paper due to use of priority queue.
   */
  std::vector<Vertex> compute(const int maxHeight);

  /**
     Same as 'compute(maxHeight)' but only considers subgraph spanned by
     vertices in 'alive'.
   */
  std::vector<Vertex> compute(const int maxHeight,
                              const std::unordered_set<Vertex> &alive);

  /**
     Set all flow, sinks and source capacities to 0.
   */
  void reset();

  /**
     Set all flow, sinks and source capacities of a subset of vertices to 0.
   */
  template <typename It> void reset(const It begin, const It end) {
    for (auto it = begin; it != end; ++it) {
      for (auto edge : graph.edges(*it))
        edge.flow = 0;
      absorbed[*it] = 0;
      sink[*it] = 0;
      nextEdgeIdx[*it] = 0;
    }
  }

  /**
     Compute a matching between vertices using the current state of the flow
     graph. A matching between vertices (u,v) is possible iff there is a path
     from u to v in the flow graph, where for each edge, the number of matchings
     going across it is <= to the flow going across it.

     Method will mutate the flow such that it is no longer legal.
   */
  std::vector<std::pair<Vertex, Vertex>>
  matching(const std::vector<Vertex> &sources);
};
