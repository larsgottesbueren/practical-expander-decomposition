#pragma once

#include <random>
#include <vector>

#include "datastructures/undirected_graph.hpp"
#include "datastructures/unit_flow.hpp"
#include "util.hpp"
#include "datastructures/priority_queue.hpp"

class PersonalizedPageRank {
public:
  using Vertex = UnitFlow::Vertex;
  void SetGraph(UnitFlow::Graph& graph_);
  void Compute(Vertex seed);
  struct PageRankAndNode {
    PageRankAndNode(double pr_, Vertex u_) : pr(pr_), u(u_) {}
    double pr;
    Vertex u;
  };
  std::vector<PageRankAndNode> ExtractSparsePageRankValues();

  struct Parameters {
    double alpha = 0.1;
    double epsilon = 1e-9;
  };
  Parameters params;
private:
  UnitFlow::Graph *graph;
  std::vector<double> page_rank;
  std::vector<double> residual;
  std::vector<Vertex> queue;
};

class Nibble {
public:
  using Vertex = UnitFlow::Vertex;
  void SetGraph(UnitFlow::Graph& graph_);
  std::pair<double, std::vector<Vertex>> ComputeCut(Vertex seed);
  void SetParams(PersonalizedPageRank::Parameters params) { ppr.params = params; }
private:
  UnitFlow::Graph *graph;
  PersonalizedPageRank ppr;
  std::vector<bool> in_cut;
  double total_vol = 0.0;
};

class LocalSearch {
public:
  using Vertex = UnitFlow::Vertex;
  void SetGraph(UnitFlow::Graph& graph_) {
    graph = &graph_;
    if (affinity_to_cluster.size() < graph->size()) {
      affinity_to_cluster.resize(graph->size(), 0);
      in_cluster.resize(graph->size(), false);
    }
  }
  void Compute(std::vector<Vertex>& seed_cluster) {
    std::cout << "Start local search on cluster of size " << seed_cluster.size() << std::endl;

    // Initialize data structures
    for (Vertex u : seed_cluster) {
      in_cluster[u] = true;
      for (auto e = graph->beginEdge(u); e != graph->endEdge(u); ++e) {
        affinity_to_cluster[e->to]++;
      }
    }
    for (Vertex u : seed_cluster) {
      pq.insert(u, RemoveVertexConductanceGain(u));
    }

    std::vector<Vertex> fruitless_moves;
    double best_conductance = Conductance(curr_cluster_cut, curr_cluster_vol);

    while (!pq.empty() && fruitless_moves.size() < max_fruitless_moves) {
      Vertex u = pq.top();

      // TODO add double-check mechanism because only neighbors receive the volume update

      double old_conductance = Conductance(curr_cluster_cut, curr_cluster_vol);
      MoveNode<true>(u);
      double new_conductance = Conductance(curr_cluster_cut, curr_cluster_vol);

      std::cout << "Moved node " << u << " diff " << old_conductance - new_conductance << std::endl;
      if (new_conductance <= best_conductance) {
        fruitless_moves.clear();
        best_conductance = new_conductance;
      } else {
        fruitless_moves.push_back(u);
      }
    }

    // revert fruitless moves
    for (Vertex u : fruitless_moves) {
      MoveNode<false>(u);
    }

    // reset datastructures
    for (size_t i = 0; i < pq.size(); ++i) {
      Vertex u = pq.at(i);
      affinity_to_cluster[u] = 0;
      in_cluster[u] = false;
    }
    pq.clear();
  }

private:
  template<bool update_pq>
  void MoveNode(Vertex u) {
    int multiplier = in_cluster[u] ? -1 : 1;
    in_cluster[u] = !in_cluster[u];
    curr_cluster_vol += multiplier * graph->degree(u);
    curr_cluster_cut += multiplier * (graph->degree(u) - 2 * affinity_to_cluster[u]);
    for (auto e = graph->beginEdge(u); e != graph->endEdge(u); ++e) {
      Vertex v = e->to;
      if (in_cluster[v]) {
        affinity_to_cluster[v] += multiplier;
      } else {
        affinity_to_cluster[v] -= multiplier;
      }
      if constexpr (update_pq) { PQUpdate(v); }
    }
    if constexpr (update_pq) { PQUpdate(u); }
  }

  void PQUpdate(Vertex v) {
    double gain = in_cluster[v] ? RemoveVertexConductanceGain(v) : AddVertexConductanceGain(v);
    pq.insertOrAdjustKey(v, gain);
  }

  double Conductance(double cut, double vol) const {
    return cut / std::min(vol, total_vol - vol);
  }

  double ConductanceGain(double new_cut, double new_vol) const {
    return Conductance(curr_cluster_cut, curr_cluster_vol) - Conductance(new_cut, new_vol) ;
  }

  double ComputeAffinityToCluster(Vertex u) const {
    double aff = 0.0;
    for (auto edge = graph->beginEdge(u); edge != graph->endEdge(u); ++edge) {
      aff += static_cast<int>(in_cluster[edge->to]);
    }
    return aff;
  }

  double AddVertexConductanceGain(Vertex u) const {
    assert(!in_cluster[u]);
    double removed_cut_edges = affinity_to_cluster[u];
    double new_cut_edges = graph->degree(u) - removed_cut_edges;
    double new_cut = curr_cluster_cut + new_cut_edges - removed_cut_edges;
    double new_vol = curr_cluster_vol + graph->degree(u);
    return ConductanceGain(new_cut, new_vol);
  }

  double RemoveVertexConductanceGain(Vertex u) const {
    assert(in_cluster[u]);
    double new_cut_edges = affinity_to_cluster[u];
    double removed_cut_edges = graph->degree(u) - new_cut_edges;
    double new_cut = curr_cluster_cut + new_cut_edges - removed_cut_edges;
    double new_vol = curr_cluster_vol - graph->degree(u);
    return ConductanceGain(new_cut, new_vol);
  }

  mt_kahypar::ds::MaxHeap<double, Vertex> pq;
  std::vector<bool> in_cluster;
  std::vector<double> affinity_to_cluster;
  UnitFlow::Graph *graph;
  double total_vol = 0.0;
  double curr_cluster_vol = 0.0;
  double curr_cluster_cut = 0.0;

  size_t max_fruitless_moves = 200;

  // TODO add tabu search
};

class SparseCutHeuristics {


private:
  Nibble nibble;
  LocalSearch local_search;
  // MQI / Trim
};
