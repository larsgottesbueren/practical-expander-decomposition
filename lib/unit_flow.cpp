#include "unit_flow.hpp"

namespace UnitFlow {

Edge::Edge(const UnitFlow::Vertex from, const UnitFlow::Vertex to, Flow flow,
           Flow capacity)
    : from(from), to(to), reverse(nullptr), flow(flow), capacity(capacity) {}

Graph::Graph(int n)
    : PartitionGraph<int, Edge>(n), absorbed(n), sink(n), height(n),
      nextEdgeIdx(n) {}

bool Graph::addEdge(Vertex u, Vertex v, Flow capacity) {
  auto p = addDirectedEdge({u, v, 0, capacity}, true);
  if (p) {
    auto rp = addDirectedEdge({v, u, 0, capacity}, false);
    assert(rp && "Reverse edge should exist if regular edge exists.");
    p->reverse = rp;
    rp->reverse = p;
    return true;
  } else {
    return false;
  }
}

std::vector<Vertex> Graph::compute(const int maxHeight) {
  std::unordered_set<int> vertices;
  for (UnitFlow::Vertex u = 0; u < size(); ++u)
    vertices.insert(u);
  return compute(maxHeight, vertices);
}

std::vector<Vertex> Graph::compute(const int maxHeight,
                                   const std::unordered_set<Vertex> &alive) {
  typedef std::pair<Flow, Vertex> QPair;
  std::priority_queue<QPair, std::vector<QPair>, std::greater<QPair>> q;

  // TODO: Is '2*alive.size()' correct?
  const int maxH = std::min(maxHeight, 2 * (int)alive.size() + 1);

  for (auto u : alive)
    if (excess(u) > 0)
      q.push({height[u], u});

  while (!q.empty()) {
    auto [_, u] = q.top();

    if (degree(u) == 0 || alive.find(u) == alive.end()) {
      q.pop();
      continue;
    }

    auto &e = edges(u)[nextEdgeIdx[u]];
    if (excess(e->from) > 0 && residual(*e) > 0 &&
        height[e->from] == height[e->to] + 1 &&
        alive.find(e->to) != alive.end()) {
      // push
      assert(excess(e->to) == 0 && "Pushing to vertex with non-zero excess");
      UnitFlow::Flow delta = std::min(
          {excess(e->from), residual(*e), (UnitFlow::Flow)degree(e->to)});

      e->flow += delta;
      absorbed[e->from] -= delta;

      e->reverse->flow -= delta;
      absorbed[e->to] += delta;

      assert(excess(e->from) >= 0 && "Excess after pushing cannot be negative");
      if (height[e->from] >= maxH || excess(e->from) == 0)
        q.pop();
      if (height[e->to] < maxH && excess(e->to) > 0)
        q.push({height[e->to], e->to});
    } else if (nextEdgeIdx[e->from] == (int)edges(e->from).size() - 1) {
      // all edges have been tried, relabel
      q.pop();
      height[e->from]++;
      nextEdgeIdx[e->from] = 0;

      if (height[e->from] < maxH)
        q.push({height[e->from], e->from});
    } else {
      nextEdgeIdx[e->from]++;
    }
  }

  std::vector<UnitFlow::Vertex> levelCut;
  for (auto u : alive)
    if (excess(u) > 0)
      levelCut.push_back(u);

  return levelCut;
}

void Graph::reset() {
  for (Vertex u = 0; u < size(); ++u) {
    for (auto &edge : edges(u))
      edge->flow = 0;
    absorbed[u] = 0;
    sink[u] = 0;
    height[u] = 0;
    nextEdgeIdx[u] = 0;
  }
}

std::vector<std::pair<Vertex, Vertex>>
Graph::matching(const std::vector<Vertex> &sources) {
  using Match = std::pair<Vertex, Vertex>;
  std::vector<Match> matches;

  std::function<Vertex(Vertex)> search = [&](Vertex start) {
    std::unordered_set<Vertex> visited;
    auto isVisited = [&visited](Vertex u) {
      return visited.find(u) != visited.end();
    };
    std::function<Vertex(Vertex)> dfs = [&](Vertex u) {
      if (isVisited(u))
        return -1;
      visited.insert(u);
      for (auto &e : edges(u)) {
        if (e->flow <= 0)
          continue;
        else if (flowIn(e->to) > 0 && sink[e->to] > 0) {
          e->flow--, absorbed[e->to]--;
          return e->to;
        } else {
          const Vertex match = dfs(e->to);
          if (match != -1) {
            e->flow--;
            return match;
          }
        }
      }
      return -1;
    };
    return dfs(start);
  };

  for (auto u : sources) {
    const Vertex v = search(u);
    if (v != -1)
      matches.push_back({u, v});
  }

  return matches;
}
} // namespace UnitFlow
