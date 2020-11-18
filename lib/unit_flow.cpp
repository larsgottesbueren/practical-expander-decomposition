#include "unit_flow.hpp"

namespace UnitFlow {

Edge::Edge(const UnitFlow::Vertex from, const UnitFlow::Vertex to, Flow flow,
           Flow capacity)
    : from(from), to(to), reverse(nullptr), flow(flow), capacity(capacity) {}

Graph::Graph(int n)
    : PartitionGraph<int, Edge>(n), absorbed(n), sink(n), height(n),
      nextEdgeIdx(n), forest(n) {}

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
  absl::flat_hash_set<int> vertices;
  for (UnitFlow::Vertex u = 0; u < size(); ++u)
    vertices.insert(u);
  return compute(maxHeight, vertices);
}

std::vector<Vertex> Graph::compute(const int maxHeight,
                                   const absl::flat_hash_set<Vertex> &alive) {
  typedef std::pair<Flow, Vertex> QPair;
  std::priority_queue<QPair, std::vector<QPair>, std::greater<QPair>> q;

  // TODO: Is '2*alive.size()' correct?
  const int maxH = std::min(maxHeight, (int)alive.size() * 2 + 1);

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

  std::vector<UnitFlow::Vertex> hasExcess;
  for (auto u : alive)
    if (excess(u) > 0)
      hasExcess.push_back(u);

  return hasExcess;
}

std::vector<Vertex> Graph::levelCut(const int maxHeight,
                                    const absl::flat_hash_set<Vertex> &alive) {
  const int h = maxHeight;
  int m = 0;
  for (auto u : alive)
    m += degree(u);
  m /= 2;

  std::vector<std::vector<Vertex>> levels(h + 1);
  for (auto u : alive)
    levels[height[u]].push_back(u);

  std::vector<Vertex> curResult;
  std::vector<Vertex> bestResult;
  int volume = 0;
  int bestZ = INT_MAX;
  for (int level = maxHeight; level > 0; --level) {
    int z = 0;
    for (auto u : levels[level]) {
      volume += degree(u);
      curResult.push_back(u);
      for (const auto &e : edges(u))
        if (alive.find(e->to) != alive.end() && height[u] == height[e->to] + 1)
          z++;
    }
    if ((double)z <= 5.0 * volume * std::log(m) / (double)h)
      if (z < bestZ)
        bestZ = z, bestResult = curResult;
  }

  return bestResult;
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
Graph::matching(const absl::flat_hash_set<Vertex> &alive,
                const std::vector<Vertex> &sources,
                const std::vector<Vertex> &targets) {
  forest.reset(alive.begin(), alive.end());

  using Match = std::pair<Vertex, Vertex>;
  std::vector<Match> matches;

  auto search = [&](Vertex start) {
    absl::flat_hash_set<Vertex> visited;
    std::vector<Edge *> path;

    std::function<Vertex(Vertex)> dfs = [&](Vertex u) {
      visited.insert(u);

      if (absorbed[u] > 0 && sink[u] > 0) {
        absorbed[u]--, sink[u]--;
        return u;
      }

      for (auto &e : edges(u)) {
        int v = e->to;
        if (e->flow <= 0 || alive.find(v) == alive.end() ||
            visited.find(v) != visited.end())
          continue;

        path.push_back(e.get());
        int m = dfs(v);
        if (m != -1)
          return m;
        path.pop_back();
      }

      return -1;
    };

    int m = dfs(start);
    if (m != -1) {
      for (auto e : path)
        e->flow--;
    }
    return m;
  };

  for (auto u : sources) {
    int m = search(u);
    if (m != -1)
      matches.push_back({u, m});
  }

  return matches;
}
} // namespace UnitFlow
