#ifndef CLUSTERING_UNIT_FLOW
#define CLUSTERING_UNIT_FLOW

#include <algorithm>
#include <list>
#include <queue>
#include <unordered_map>
#include <vector>

/**
   Push relabel based unit flow algorithm. Based on push relabel in KACTL.
 */

using Vertex = int;
using Flow = long long;

struct UnitFlow {
public:
  struct Edge {
    Vertex from, to;
    /**
        Index such that graph[to][backIdx] = edge to->from
    */
    int backIdx;
    Flow flow, capacity;

    Edge() {}
    Edge(Vertex from, Vertex to, int backIdx = -1, Flow flow = 0, Flow capacity = 0)
      : from(from), to(to), backIdx(backIdx), flow(flow), capacity(capacity) {}
  };

private:

  /**
     For each vertex maintain a neighbor list.
   */
  std::vector<std::vector<Edge>> graph;
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
     Maximum label height.
   */
  const int maxHeight;

  /**
     The degree of a vertex.
   */
  int degree(Vertex u) const { return (int)graph[u].size(); }

  /**
     The number of vertices in the flow graph.
   */
  int size() const { return (int)graph.size(); }

  /**
     Return the excess of a node, i.e. the flow it cannot absorb.
   */
  Flow excess(Vertex u) const {
    return std::max((Flow)0, absorbed[u] - sink[u]);
  }

  /**
     Residual capacity of an edge.
   */
  Flow residual(const Edge & e) const {
    return e.capacity - e.flow;
  }

public:
  /**
     Construct a unit flow problem with n vertices and max height h.
   */
  UnitFlow(int n, int maxHeight) : graph(n), absorbed(n), sink(n), height(n), nextEdgeIdx(n), maxHeight(maxHeight) {}

  void addEdge(Vertex u, Vertex v, Flow capacity) {
    if (u == v)
      return;
    int uNeighborCount = (int)graph[u].size(),
      vNeighborCount = (int)graph[v].size();

    graph[u].emplace_back(u, v, vNeighborCount, 0, capacity);
    graph[v].emplace_back(v, u, uNeighborCount, 0, capacity);
  }

  /**
     Increase the amount of flow a vertex is currently absorbing.
   */
  void addSource(Vertex u, Flow amount) {
    absorbed[u] += amount;
  }

  /**
     Increase the amount of flow a vertex is able to absorb on its own.
   */
  void addSink(Vertex u, Flow amount) {
    sink[u] += amount;
  }

  /**
      The amount of flow absorbed by a vertex.
  */
  Flow getAbsorbed(Vertex u) const { return absorbed[u]; }

  /**
     Compute max flow with push relabel and max height h. Return those vertices
     part of a level cut (height[u] >= h). If an empty vector is returned then
     all flow was possible to route.

     Extra log factor compared to paper due to use of priority queue.
   */
  std::vector<Vertex> compute() {
    typedef std::pair<Flow,Vertex> QPair;
    std::priority_queue<QPair,std::vector<QPair>,std::greater<QPair>> q;

    const int maxH = std::min(maxHeight, 2 * size() + 1);

    for (Vertex u = 0; u < size(); ++u)
      if (absorbed[u] > sink[u])
        q.push({height[u],u});

    while (!q.empty()) {
      auto [_,u] = q.top();

      Edge & e = graph[u][nextEdgeIdx[u]];
      if (excess(e.from) > 0 && residual(e) > 0 && height[e.from] == height[e.to] + 1) {
        // push
        Flow delta = std::min({excess(e.from), residual(e)}); // TODO: Add push limit based on degree

        e.flow += delta;
        absorbed[e.from] -= delta;

        graph[e.to][e.backIdx].flow -= delta;
        absorbed[e.to] += delta;

        assert(excess(e.from) >= 0 && "Excess after pushing cannot be negative");
        if (height[e.from] >= maxH || excess(e.from) == 0)
          q.pop();
        if (height[e.to] < maxH && excess(e.to))
          q.push({height[e.to],e.to});
      } else if (nextEdgeIdx[e.from] == (int)graph[e.from].size() - 1) {
        // all edges have been tried, relabel
        q.pop();
        height[e.from]++;
        nextEdgeIdx[e.from] = 0;

        if (height[e.from] < maxH)
          q.push({height[e.from],e.from});
      } else {
        nextEdgeIdx[e.from]++;
      }
    }

    return {};
  }
};

#endif
