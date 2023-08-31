#include "unit_flow.hpp"
#include <functional>
#include <util.hpp>
#include <chrono>

namespace UnitFlow {

Edge::Edge(Vertex from, Vertex to, Flow flow, Flow capacity, Flow congestion)
    : from(from), to(to), revIdx(-1), flow(flow), capacity(capacity),
      congestion(congestion) {}

Edge::Edge(Vertex from, Vertex to, Flow flow, Flow capacity)
    : Edge(from, to, flow, capacity, 0) {}

Edge::Edge(Vertex from, Vertex to, Flow capacity)
    : Edge(from, to, 0, capacity) {}

Graph::Graph(int n, const std::vector<Edge> &es)
    : SubsetGraph::Graph<int, Edge>(n, es), absorbed(n), sink(n), height(n),
      nextEdgeIdx(n) {}

std::vector<Vertex> Graph::compute(const int maxHeight) {
  const int maxH = std::min(maxHeight, size() * 2 + 1);

  std::vector<std::queue<Vertex>> q(maxH + 1);

  for (auto u : *this)
    if (excess(u) > 0)
      q[0].push(u);

  int level = 0;

  while (level <= maxH) {
    if (q[level].empty()) {
      level++;
      continue;
    }

    const int u = q[level].front();
    if (degree(u) == 0) {
      q[level].pop();
      continue;
    }

    assert(excess(u) > 0 &&
           "Vertex popped from queue should have excess flow.");

    auto &e = getEdge(u, nextEdgeIdx[u]);

    assert(e.flow + reverse(e).flow == 0 &&
           "Flow across edge and its reverse should cancel.");

    if (e.residual() > 0 && height[u] == height[e.to] + 1) {
      // Push flow across 'e'
      assert(excess(e.to) == 0 && "Pushing to vertex with non-zero excess");
      UnitFlow::Flow delta = std::min(
          {excess(e.from), e.residual(), (UnitFlow::Flow)degree(e.to)});

      e.flow += delta;
      reverse(e).flow -= delta;

      absorbed[e.from] -= delta;
      absorbed[e.to] += delta;

      assert(excess(e.from) >= 0 && "Excess after pushing cannot be negative");
      if (height[e.from] >= maxH || excess(e.from) == 0)
        q[level].pop();

      if (height[e.to] < maxH && excess(e.to) > 0) {
        q[height[e.to]].push(e.to);
        level = std::min(level, height[e.to]);       // You go to the level of the node you just pushed to next????? Why???
        nextEdgeIdx[e.to] = 0;
      }
    } else if (nextEdgeIdx[e.from] == degree(e.from) - 1) {
      // all edges have been tried, relabel
      q[level].pop();
      height[e.from]++;// ???? What the fuck is this? TODO relabel properly
        // TODO the current-arc data structure is almost useless since the same node is pushed repeatedly until it has to be relabeled or its excess is gone
        // The same could be achieved more efficiently with a proper discharge implementation
        //

        nextEdgeIdx[e.from] = 0;

      if (height[e.from] < maxH)
        q[height[e.from]].push(e.from);
    } else {
      nextEdgeIdx[e.from]++;
    }
  }

  for (auto u : *this)
    for (auto e = beginEdge(u); e != endEdge(u); ++e)
      if (e->flow > 0)
        e->congestion += e->flow;

  std::vector<UnitFlow::Vertex> hasExcess;
  for (auto u : *this)
    if (excess(u) > 0)
      hasExcess.push_back(u);

  return hasExcess;
}

std::pair<std::vector<Vertex>, std::vector<Vertex>>
Graph::levelCut(const int h) {
  std::vector<std::vector<Vertex>> levels(h + 1);
  for (auto u : *this)
    levels[height[u]].push_back(u);

  int volume = 0;
  // int bestZ = INT_MAX;
  double bestConductance = 1.0;
  int bestLevel = h;
  for (int level = h; level > 0; --level) {
    int z = 0;
    for (auto u : levels[level]) {
      volume += degree(u);
      for (auto e = beginEdge(u); e != endEdge(u); ++e)
        if (height[u] == height[e->to] + 1)
          z++;
    }
    double conductance =
        double(z) / double(std::min(volume, this->volume() - volume));
    if (conductance < bestConductance)
      bestConductance = conductance, bestLevel = level;
  }

  std::vector<int> left, right;
  for (int level = h; level >= bestLevel; --level)
    for (auto u : levels[level])
      left.push_back(u);
  for (int level = 0; level < bestLevel; ++level)
    for (auto u : levels[level])
      right.push_back(u);

  return std::make_pair(left, right);
}

void Graph::reset() {
  for (auto u : *this) {
    for (auto e = beginEdge(u); e != endEdge(u); ++e)
      e->flow = 0;
    absorbed[u] = 0;
    sink[u] = 0;
    height[u] = 0;
    nextEdgeIdx[u] = 0;
  }
}

std::vector<std::pair<Vertex, Vertex>>
Graph::matchingDfs(const std::vector<Vertex> &sources) {
  std::vector<std::pair<Vertex, Vertex>> matches;

  auto search = [&](Vertex start) {
    std::vector<Edge *> path;
    std::function<Vertex(Vertex)> dfs = [&](Vertex u) {
      visited[u] = start + 1;

      if (absorbed[u] > 0 && sink[u] > 0) {
        absorbed[u]--, sink[u]--;
        return u;
      }

      for (auto e = beginEdge(u); e != endEdge(u); ++e) {
        int v = e->to;
        if (e->flow <= 0 || visited[v] == start + 1)
          continue;

        path.push_back(&*e);
        int m = dfs(v);
        if (m != -1)
          return m;
        path.pop_back();
      }

      return -1;
    };

    int m = dfs(start);
    if (m != -1)
      for (auto e : path)
        e->flow--;
    return m;
  };

  for (auto u : sources) {
    int m = search(u);
    if (m != -1)
      matches.push_back({u, m});
  }

  for (auto it = cbegin(); it != cend(); ++it)
    visited[*it] = false;

  return matches;
}


std::vector<std::pair<Vertex, Vertex>>
Graph::matching(const std::vector<Vertex> &sources) {
    return matchingDfs(sources);
}
} // namespace UnitFlow
