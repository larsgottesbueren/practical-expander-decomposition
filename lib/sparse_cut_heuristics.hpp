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
  double alpha = 0.15;
  double epsilon = 1e-12;
  void SetGraph(UnitFlow::Graph& graph_);
  void Compute(Vertex seed);
  struct PageRankAndNode {
    PageRankAndNode(double pr_, Vertex u_) : pr(pr_), u(u_) {}
    double pr;
    Vertex u;
  };
  std::vector<PageRankAndNode> ExtractSparsePageRankValues();
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
  void SetGraph(UnitFlow::Graph& graph_);
  std::optional<std::vector<Vertex>> ComputeCut(Vertex seed);
private:
  UnitFlow::Graph *graph;
  PersonalizedPageRank ppr;
  std::vector<bool> in_cut;
  double total_vol = 0.0;
};

class LocalSearch {
public:
  using Vertex = UnitFlow::Vertex;
private:
  mt_kahypar::ds::MinHeap<Vertex, double> pq;
};

class SparseCutHeuristics {


private:
  Nibble nibble;
  LocalSearch local_search;
};
