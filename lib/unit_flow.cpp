#include "unit_flow.hpp"
#include <iostream>

UnitFlow::Edge::Edge(const UnitFlow::Vertex from, const UnitFlow::Vertex to,
                     const int backIdx, UnitFlow::Flow flow = 0,
                     UnitFlow::Flow capacity = 0)
    : from(from), to(to), backIdx(backIdx), flow(flow), capacity(capacity) {}

UnitFlow::Edge UnitFlow::Edge::rev() const {
  Edge e(to, from, 0, flow, capacity);
  return e;
}

UnitFlow::UnitFlow(int n)
    : graph(n), absorbed(n), sink(n), height(n), nextEdgeIdx(n) {}

void UnitFlow::addEdge(UnitFlow::Vertex u, UnitFlow::Vertex v,
                       UnitFlow::Flow capacity) {
  if (u == v)
    return;

  graph.addEdge({u, v, graph.degree(u), 0, capacity});
  graph.addEdge({v, u, graph.degree(v), 0, capacity});
}

std::vector<UnitFlow::Vertex> UnitFlow::compute(const int maxHeight) {
  typedef std::pair<UnitFlow::Flow, UnitFlow::Vertex> QPair;
  std::priority_queue<QPair, std::vector<QPair>, std::greater<QPair>> q;

  const int maxH = std::min(maxHeight, 2 * graph.size() + 1);

  for (UnitFlow::Vertex u = 0; u < graph.size(); ++u)
    if (excess(u) > 0)
      q.push({height[u], u});

  while (!q.empty()) {
    auto [_, u] = q.top();

    if (graph.degree(u) == 0) {
      q.pop();
      continue;
    }

    Edge &e = graph.edges(u)[nextEdgeIdx[u]];
    if (excess(e.from) > 0 && residual(e) > 0 &&
        height[e.from] == height[e.to] + 1) {
      // push
      assert(excess(e.to) == 0 && "Pushing to vertex with non-zero excess");
      UnitFlow::Flow delta = std::min(
          {excess(e.from), residual(e), (UnitFlow::Flow)graph.degree(e.to)});

      e.flow += delta;
      absorbed[e.from] -= delta;

      graph.edges(e.to)[e.backIdx].flow -= delta;
      absorbed[e.to] += delta;

      assert(excess(e.from) >= 0 && "Excess after pushing cannot be negative");
      if (height[e.from] >= maxH || excess(e.from) == 0)
        q.pop();
      if (height[e.to] < maxH && excess(e.to) > 0)
        q.push({height[e.to], e.to});
    } else if (nextEdgeIdx[e.from] == (int)graph.edges(e.from).size() - 1) {
      // all edges have been tried, relabel
      q.pop();
      height[e.from]++;
      nextEdgeIdx[e.from] = 0;

      if (height[e.from] < maxH)
        q.push({height[e.from], e.from});
    } else {
      nextEdgeIdx[e.from]++;
    }
  }

  std::vector<UnitFlow::Vertex> levelCut;
  for (UnitFlow::Vertex u = 0; u < graph.size(); ++u)
    if (excess(u) > 0)
      levelCut.push_back(u);

  return levelCut;
}

std::vector<UnitFlow::Vertex>
UnitFlow::compute(const int maxHeight,
                  const std::unordered_set<Vertex> &alive) {
  typedef std::pair<UnitFlow::Flow, UnitFlow::Vertex> QPair;
  std::priority_queue<QPair, std::vector<QPair>, std::greater<QPair>> q;

  // TODO: Is '2*alive.size()' correct?
  const int maxH = std::min(maxHeight, 2 * (int)alive.size() + 1);

  for (auto u : alive)
    if (excess(u) > 0)
      q.push({height[u], u});

  while (!q.empty()) {
    auto [_, u] = q.top();

    if (graph.partitionDegree(u) == 0 || alive.find(u) == alive.end()) {
      q.pop();
      continue;
    }

    Edge &e = graph.partitionEdges(u)[nextEdgeIdx[u]];
    if (excess(e.from) > 0 && residual(e) > 0 &&
        height[e.from] == height[e.to] + 1 && alive.find(e.to) != alive.end()) {
      // push
      assert(excess(e.to) == 0 && "Pushing to vertex with non-zero excess");
      UnitFlow::Flow delta =
          std::min({excess(e.from), residual(e),
                    (UnitFlow::Flow)graph.partitionDegree(e.to)});

      e.flow += delta;
      absorbed[e.from] -= delta;

      graph.partitionEdges(e.to)[e.backIdx].flow -= delta;
      absorbed[e.to] += delta;

      assert(excess(e.from) >= 0 && "Excess after pushing cannot be negative");
      if (height[e.from] >= maxH || excess(e.from) == 0)
        q.pop();
      if (height[e.to] < maxH && excess(e.to) > 0)
        q.push({height[e.to], e.to});
    } else if (nextEdgeIdx[e.from] ==
               (int)graph.partitionEdges(e.from).size() - 1) {
      // all edges have been tried, relabel
      q.pop();
      height[e.from]++;
      nextEdgeIdx[e.from] = 0;

      if (height[e.from] < maxH)
        q.push({height[e.from], e.from});
    } else {
      nextEdgeIdx[e.from]++;
    }
  }

  std::vector<UnitFlow::Vertex> levelCut;
  for (auto u : alive)
    if (excess(u) > 0)
      levelCut.push_back(u);

  return levelCut;
}

void UnitFlow::reset() {
  for (UnitFlow::Vertex u = 0; u < graph.size(); ++u) {
    for (auto edge : graph.edges(u))
      edge.flow = 0;
    absorbed[u] = 0;
    sink[u] = 0;
    nextEdgeIdx[u] = 0;
  }
}

std::vector<std::pair<UnitFlow::Vertex, UnitFlow::Vertex>>
UnitFlow::matching(const std::vector<UnitFlow::Vertex> &sources) {
  using Match = std::pair<UnitFlow::Vertex, UnitFlow::Vertex>;
  std::vector<Match> matches;

  std::function<UnitFlow::Vertex(UnitFlow::Vertex)> search =
      [&](UnitFlow::Vertex start) {
        std::unordered_set<UnitFlow::Vertex> visited;
        auto isVisited = [&visited](UnitFlow::Vertex u) {
          return visited.find(u) != visited.end();
        };
        std::function<UnitFlow::Vertex(UnitFlow::Vertex)> dfs =
            [&](UnitFlow::Vertex u) {
              if (isVisited(u))
                return -1;
              visited.insert(u);
              for (auto &e : graph.edges(u)) {
                if (e.flow <= 0)
                  continue;
                else if (flowIn(e.to) > 0 && sink[e.to] > 0) {
                  e.flow--, absorbed[e.to]--;
                  return e.to;
                } else {
                  const UnitFlow::Vertex match = dfs(e.to);
                  if (match != -1) {
                    e.flow--;
                    return match;
                  }
                }
              }
              return -1;
            };
        return dfs(start);
      };

  for (auto u : sources) {
    const UnitFlow::Vertex v = search(u);
    if (v != -1)
      matches.push_back({u, v});
  }

  return matches;
}
