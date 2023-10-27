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

class Nibble {
public:
  using Vertex = UnitFlow::Vertex;
  double alpha = 0.15;
  double epsilon = 1e-12;
  double conductance_goal = 1e-3;

  void SetGraph(UnitFlow::Graph& graph_) {
    graph = &graph_;
    ppr.SetGraph(graph_);
    if (in_cut.size() < graph->size()) {
      in_cut.resize(graph->size());
    }
    total_vol = graph->volume();
  }

  std::optional<std::vector<Vertex>> ComputeCut(Vertex seed) {
    ppr.Compute(seed);
    auto ppr_distr = ppr.ExtractSparsePageRankValues();
    for (auto& pru : ppr_distr) {
      pru.pr = pru.pr / graph->degree(pru.u);
    }
    std::sort(ppr_distr.begin(), ppr_distr.end(), [](const auto& l, const auto& r) { return l.pr > r.pr; });

    double cut = 0;
    double vol = 0;
    double best_conductance = std::numeric_limits<double>::max();
    int best_cut_index = -1;

    for (int i = 0; i < ppr_distr.size(); ++i) {
      const Vertex u = ppr_distr[i].u;
      vol += graph->degree(u);

      for (auto e = graph->beginEdge(u); e != graph->endEdge(u); ++e) {
        Vertex v = e->to;
        if (in_cut[v]) {
          cut -= 1;
        } else {
          cut += 1;
        }
      }

      const double conductance = cut / std::min(vol, total_vol - vol);
      if (conductance < best_conductance) {
        best_conductance = conductance;
        best_cut_index = i;
      }
    }

    if (best_conductance <= conductance_goal) {
      std::vector<Vertex> cutset;
      for (int i = 0; i <= best_cut_index; ++i) {
        cutset.push_back(ppr_distr[i].u);
      }
      return cutset;
    } else {
      return std::nullopt;
    }
  }

private:
  UnitFlow::Graph *graph;
  PersonalizedPageRank ppr;
  std::vector<bool> in_cut;
  double total_vol = 0.0;
};

class LocalSearch {

};

class SparseCutHeuristics {


private:
  Nibble nibble;
  LocalSearch local_search;
};
