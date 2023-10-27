#pragma once

#include <random>
#include <vector>

#include "datastructures/undirected_graph.hpp"
#include "datastructures/unit_flow.hpp"
#include "util.hpp"

// TODO move some code into the cpp file

class PersonalizedPageRank {
public:
  using Vertex = UnitFlow::Vertex;
  double alpha = 0.15;
  double epsilon = 1e-12;

  void SetGraph(UnitFlow::Graph& graph_) {
    graph = &graph_;
    if (page_rank.size() < graph->size()) {
      page_rank.resize(graph->size());
      residual.resize(graph->size());
    }
  }

  void Compute(Vertex seed) {
    // clear old queue
    for (Vertex u : queue) {
      residual[u] = 0.0;
      page_rank[u] = 0.0;
    }
    queue.clear();

    // add new seed
    queue.push_back(seed);
    residual[seed] = 1.0;

    // push loop
    for (size_t i = 0; i < queue.size(); i++) {
      const Vertex u = queue[i];
      const double res_u = residual[u];
      const double mass_preserved = (1.0-alpha)*res_u/2;
      const double mass_pushed_to_neighbors = mass_preserved / graph->degree(u);  // TODO beware. do we need the volume in the surrounding graph?

      for (auto e = graph->beginEdge(u); e != graph->endEdge(u); ++e) {
        const Vertex v = e->to;
        const double insert_threshold = epsilon * graph->degree(v);
        if (residual[v] < insert_threshold && residual[v] + mass_pushed_to_neighbors >= insert_threshold) {
          queue.push_back(v);
        }
        residual[v] += mass_pushed_to_neighbors;
      }

      page_rank[u] += alpha * res_u;
      residual[u] = mass_preserved;
    }
  }


  struct PageRankAndNode {
    PageRankAndNode(double pr_, Vertex u_) : pr(pr_), u(u_) {}
    double pr;
    Vertex u;
  };
  std::vector<PageRankAndNode> ExtractSparsePageRankValues() {
    std::vector<PageRankAndNode> result;
    for (Vertex u : queue) {
      result.emplace_back(page_rank[u], u);
      residual[u] = 0.0;
      page_rank[u] = 0.0;
    }
    queue.clear();
    return result;
  }

private:
  UnitFlow::Graph *graph;

  std::vector<double> page_rank;
  std::vector<double> residual;
  std::vector<Vertex> queue;
};
