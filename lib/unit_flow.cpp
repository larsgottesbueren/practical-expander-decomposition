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
Graph::matching(const std::unordered_set<Vertex> &alive,
                const std::vector<Vertex> &sources,
                const std::vector<Vertex> &targets) {
  forest.reset(alive.begin(), alive.end());

#ifdef DEBUG
  for (auto u : sources)
    assert(alive.find(u) != alive.end() && "Source not in alive set.");
  for (auto u : targets)
    assert(alive.find(u) != alive.end() && "Target not in alive set.");
#endif

  using Match = std::pair<Vertex, Vertex>;
  std::vector<Match> matches;

  std::function<Vertex(Vertex)> dfs = [&](Vertex u) {
    assert(alive.find(u) != alive.end() && "Must search from an alive vertex.");

    int r = forest.findRoot(u);
    assert(alive.find(r) != alive.end() && "Root must be alive.");

    if (flowIn(r) > 0 && sink[r] > 0) {
      absorbed[r]--;
      return r;
    }

    for (auto &e : edges(r)) {
      if (e->flow <= 0 || forest.findRoot(e->to) == r ||
          alive.find(e->to) == alive.end())
        continue;

      assert(e->from == r && "Must relax edges from current root.");

      forest.set(e->from, 0);
      forest.link(e->from, e->to, e->flow);
      e->flow = 0;

      Vertex m = dfs(r);
      if (m != -1)
        return m;

      for (auto &f : edges(e->to))
        if (alive.find(f->to) != alive.end() &&
            forest.findParent(f->to) == e->to)
          forest.cut(f->to), forest.set(f->to, 0);
    }

    return -1;
  };

  for (auto u : targets)
    forest.set(u, 1 << 28); // TODO: Fix 'findPathMin' to avoid this constant.

  for (auto u : sources) {
    const Vertex v = dfs(u);
    if (v != -1) {
      matches.push_back({u, v});

      forest.updatePathEdges(u, -1);
      while (forest.findRoot(u) != u) {
        auto [value, w] = forest.findPathMin(u);
        if (value == 0)
          forest.cut(w), forest.set(w, 1 << 28);
        else
          break;
      }
    }
  }

  return matches;
}

/*
std::vector<std::pair<Vertex, Vertex>>
Graph::matching(const std::vector<Vertex> &subset,
              const std::vector<Vertex> &sources,
              const std::vector<Vertex> &targets) {
forest.reset(subset.begin(), subset.end());

using Match = std::pair<Vertex, Vertex>;
std::vector<Match> matches;

std::function<Vertex(Vertex)> search = [&](Vertex start) {
  std::unordered_set<Vertex> visited;
  std::function<Vertex(Vertex)> dfs = [&](Vertex u) {
    int r = forest.findRoot(u);
    while (true) {
      assert(forest.findRoot(r) == r && "Must always search from root of
path."); if (visited.find(r) != visited.end()) return -1; visited.insert(r);

      if (flowIn(r) > 0 && sink[r] > 0) {
        absorbed[r]--;
        return r;
      }

      for (auto &e : edges(r)) {
        if (e->flow <= 0 ||
            visited.find(e->to) != visited.end() ||
            forest.findRoot(e->to) == r)
          continue;

        forest.set(e->from, 0);
        forest.link(e->from, e->to, e->flow);
        e->flow = 0;

        Vertex m = dfs(e->to);
        if (m != -1)
          return m;
        else
          forest.cut(e->from), forest.set(e->from, 0);
      }

      if (r != u) {
        r = forest.findRootEdge(u);
        forest.cut(r), forest.set(r, 0);
      } else {
        break;
      }
    }
    return -1;
  };

  Vertex match = dfs(start);
  if (match != -1) {
    forest.updatePathEdges(start, -1);
    while (forest.findRoot(start) != start) {
      auto [w, u] = forest.findPathMin(start);
      if (w == 0)
        forest.cut(u), forest.set(u, 1<<28);
      else
        break;
    }
  }
  return match;
};

for (auto u : targets)
  forest.set(u, 1 << 28); // TODO: Fix 'findPathMin' to avoid this constant.
for (auto u : sources) {
  const Vertex v = search(u);
  if (v != -1)
    matches.push_back({u, v});
}

return matches;
}
*/
} // namespace UnitFlow
