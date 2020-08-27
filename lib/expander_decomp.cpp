#include "expander_decomp.hpp"

ExpanderDecomp::ExpanderDecomp(Graph g) : graph(g), partition(g.size()) {}

void ExpanderDecomp::go(double phi, std::vector<Vertex> vs) {
  CutMatching cm(graph);
  CutMatching::Result result = cm.compute(phi);

  std::vector<bool> inA(graph.size());
  for (auto u : result.a)
    inA[u] = true;

  switch (result.t) {
  case CutMatching::Balanced: {
    Graph lg(result.a.size()), rg(result.r.size());
    for (auto u : result.a)
      for (auto v : graph.neighbors[u])
        if (inA[v])
          lg.addEdge(u, v);
    for (auto u : result.r)
      for (auto v : graph.neighbors[u])
        if (!inA[v])
          rg.addEdge(u, v);

    break;
  }
  }
}

std::vector<std::vector<Vertex>> ExpanderDecomp::compute(double phi) const {
  CutMatching cm(graph);
  CutMatching::Result result = cm.compute(phi);

  std::vector<bool> inA(graph.size());
  for (auto u : result.a)
    inA[u] = true;

  switch (result.t) {
  case CutMatching::Balanced: {
    Graph lg(result.a.size());
    Graph rg(result.r.size());

    for (auto u : result.a)
      for (auto v : graph.neighbors[u])
        if (inA[v])
          lg.addEdge(u, v);
    for (auto u : result.r)
      for (auto v : graph.neighbors[u])
        if (!inA[v])
          rg.addEdge(u, v);

    ExpanderDecomp left(lg);
    ExpanderDecomp right(rg);

    std::vector<std::vector<Vertex>> result;
    break;
  }
  case CutMatching::Expander: {

    break;
  }
  case CutMatching::NearExpander: {

    break;
  }
  }
}
