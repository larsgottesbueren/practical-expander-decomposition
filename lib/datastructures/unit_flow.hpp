#pragma once

#include <algorithm>
#include <list>
#include <queue>
#include <vector>

#include "absl/container/flat_hash_set.h"
#include "linkcut.hpp"
#include "subset_graph.hpp"

namespace UnitFlow {

using Vertex = int;
using Flow = long long;

struct Edge {
  Vertex from, to, revIdx;
  Flow flow, capacity;

  Edge(Vertex from, Vertex to, Flow flow, Flow capacity);
  /**
     Construct an edge between two vertices with a certain capacity and zero
     flow.
   */
  Edge(Vertex from, Vertex to, Flow capacity);

  /**
     Residual capacity. I.e. the amount of capacity left over.
   */
  Flow residual() const { return capacity - flow; }

  /**
     Construct the reverse of this edge. 'capacity' is the same but 'flow' is
     set to zero. 'revIdx' remains undefined since it is maintained by the graph
     representation.
  */
  Edge reverse() const {
    Edge e{to, from, 0, capacity};
    return e;
  }

  /**
     Two edges are equal if all their fields agree, including reverse index.
  */
  friend bool operator==(const Edge &lhs, const Edge &rhs) {
    return lhs.from == rhs.from && lhs.to == rhs.to &&
           lhs.revIdx == rhs.revIdx && lhs.flow == rhs.flow &&
           lhs.capacity == rhs.capacity;
  }
};

/**
   Push relabel based unit flow algorithm. Based on push relabel in KACTL.
 */
class Graph : public SubsetGraph::Graph<int, Edge> {
private:
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

  /**
     A link-cut forest used for computing matchings.
   */
  LinkCut::Forest forest;

public:
  /**
     Construct a unit flow problem with 'n' vertices and edges 'es'.
   */
  Graph(int n, const std::vector<Edge> &es);

  const std::vector<Flow> &getAbsorbed() const { return absorbed; }
  const std::vector<Flow> &getSink() const { return sink; }
  const std::vector<Vertex> &getHeight() const { return height; }
  const std::vector<int> &getNextEdgeIdx() const { return nextEdgeIdx; }

  /**
     Add an undirected edge '{u,v}' with a certain capacity. If 'u = v' do
     nothing. If vertices are in separate partitions, edge is not added but
     global degree of 'u' is incremented.

     Return true if an edge was added or false otherwise.
   */
  bool addEdge(Vertex u, Vertex v, Flow capacity);

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
    for (auto e = cbeginEdge(u); e != cendEdge(u); ++e)
      if (e->flow > 0)
        f += e->flow;
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
     with excess flow left over. If an empty vector is returned then all flow
     was possible to route.
   */
  std::vector<Vertex> compute(const int maxHeight);

  /**
     Compute a level cut. See Saranurak and Wang A.1.

     Precondition: A flow has been computed.
   */
  std::vector<Vertex> levelCut(const int maxHeight);

  /**
     Set all flow, sinks and source capacities to 0.
   */
  void reset();

  /**
     Set all flow, sinks and source capacities of a subset of vertices to 0.
   */
  template <typename It> void reset(const It begin, const It end) {
    for (auto u : *this) {
      for (auto e = beginEdge(u); e != endEdge(u); ++e)
        e->flow = 0;
      absorbed[u] = 0;
      sink[u] = 0;
      height[u] = 0;
      nextEdgeIdx[u] = 0;
    }
  }

  /**
     Compute a matching between vertices using the current state of the flow
     graph. A matching between vertices (u,v) is possible iff there is a path
     from u to v in the flow graph, where for each edge, the number of matchings
     going across it is <= to the flow going across it.

     Method will mutate the flow such that it is no longer legal.

     Time complexity: O(m^2)
   */
  std::vector<std::pair<Vertex, Vertex>>
  matchingSlow(const std::vector<Vertex> &sources);

  /**
     Compute a matching between vertices using the current state of the flow
     graph. A matching between vertices (u,v) is possible iff there is a path
     from u to v in the flow graph, where for each edge, the number of matchings
     going across it is <= to the flow going across it.

     Method will mutate the flow such that it is no longer legal.

     Time complexity: O(m \log m)
   */
  std::vector<std::pair<Vertex, Vertex>>
  matching(const std::vector<Vertex> &sources);
};
} // namespace UnitFlow
