#include "sparse_cut_heuristics.hpp"

void PersonalizedPageRank::Compute(Vertex seed) {
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
    const double mass_preserved = (1.0-params.alpha)*res_u/2;
    const double mass_pushed_to_neighbors = mass_preserved / graph->degree(u);  // TODO beware. do we need the volume in the surrounding graph?

    // std::cout << "Push from " << u << " residual[u] = " << residual[u] << " mass preserved " << mass_preserved << " mass pushed " << mass_pushed_to_neighbors << std::endl;

    for (auto e = graph->beginEdge(u); e != graph->endEdge(u); ++e) {
      const Vertex v = e->to;
      const double insert_threshold = params.epsilon * graph->degree(v);
      if (residual[v] < insert_threshold && residual[v] + mass_pushed_to_neighbors >= insert_threshold) {
        queue.push_back(v);
      }
      residual[v] += mass_pushed_to_neighbors;
    }

    page_rank[u] += params.alpha * res_u;
    residual[u] = mass_preserved;
    if (residual[u] >= params.epsilon * graph->degree(u)) {
      queue.push_back(u);
    }
  }
}

std::vector<PersonalizedPageRank::PageRankAndNode> PersonalizedPageRank::ExtractSparsePageRankValues() {
  std::vector<PageRankAndNode> result;
  for (Vertex u : queue) {
    if (page_rank[u] > 0.0) {
      result.emplace_back(page_rank[u], u);
    }
    residual[u] = 0.0;
    page_rank[u] = 0.0;
  }
  queue.clear();
  return result;
}

void PersonalizedPageRank::SetGraph(UnitFlow::Graph& graph_) {
  graph = &graph_;
  if (page_rank.size() < size_t(graph->size())) {
    page_rank.resize(graph->size());
    residual.resize(graph->size());
  }
}

void Nibble::SetGraph(UnitFlow::Graph& graph_) {
  graph = &graph_;
  ppr.SetGraph(graph_);
  if (in_cut.size() < size_t(graph->size())) {
    in_cut.resize(graph->size());
  }
  total_vol = graph->volume();
}

Nibble::Cut Nibble::ComputeCut(Vertex seed) {
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

  for (int i = 0; i < int(ppr_distr.size()); ++i) {
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

    in_cut[u] = true;

    // TODO add filter / preference for min balance

    const double conductance = cut / std::min(vol, total_vol - vol);
    if (conductance < best_conductance) {
      best_conductance = conductance;
      best_cut_index = i;
    }
  }

  std::vector<Vertex> cutset;
  for (int i = 0; i <= best_cut_index; ++i) {
    Vertex u = ppr_distr[i].u;
    result.cut_side.push_back(u);
    result.volume += graph->degree(u);
  }
  result.cut = best_conductance * std::min(result.volume, total_vol - result.volume);

  std::cout << "len(PPR) = " << ppr_distr.size() << " best cut index = " << best_cut_index
            << " cut = " << result.cut << " vol = " << result.volume
            << " conductance = " << result.conductance << std::endl;

  return result;
}

void LocalSearch::SetGraph(UnitFlow::Graph& graph_) {
  graph = &graph_;
  if (affinity_to_cluster.size() < graph->size()) {
    affinity_to_cluster.resize(graph->size(), 0);
    in_cluster.resize(graph->size(), false);
    pq.resize(graph->size());
  }
  total_vol = graph->volume();
}

template<bool update_pq>
void LocalSearch::MoveNode(Vertex u) {
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

LocalSearch::Result LocalSearch::Compute(std::vector<LocalSearch::Vertex>& seed_cluster) {
  std::cout << "Start local search on cluster of size " << seed_cluster.size() << std::endl;

  // clean up old datastructures
  for (size_t i = 0; i < pq.size(); ++i) {
    Vertex u = pq.at(i);
    affinity_to_cluster[u] = 0;
    in_cluster[u] = false;
  }
  pq.clear();

  // Initialize data structures
  for (Vertex u : seed_cluster) {
    in_cluster[u] = true;
    for (auto e = graph->beginEdge(u); e != graph->endEdge(u); ++e) {
      affinity_to_cluster[e->to]++;
    }
  }
  curr_cluster_vol = 0;
  curr_cluster_cut = 0;
  for (Vertex u : seed_cluster) {
    curr_cluster_cut += graph->degree(u) - affinity_to_cluster[u];
    curr_cluster_vol += graph->degree(u);
  }

  for (Vertex u : seed_cluster) {
    pq.insert(u, RemoveVertexConductanceGain(u));
    for (auto e = graph->beginEdge(u); e != graph->endEdge(u); ++e) {
      Vertex v = e->to;
      if (!in_cluster[v] && !pq.contains(v)) {
        pq.insert(v, AddVertexConductanceGain(v));
      }
    }
  }


  std::vector<Vertex> fruitless_moves;
  double best_conductance = Conductance(curr_cluster_cut, curr_cluster_vol);
  std::cout << "Vol = " << curr_cluster_vol << " Cut = " << curr_cluster_cut << " Conductance = "
            << best_conductance << " PQ size " << pq.size() << std::endl;

  int steps = 0;



  while (!pq.empty() && fruitless_moves.size() < max_fruitless_moves) {
    Vertex u = pq.top();

    // TODO add double-check mechanism because only neighbors receive the volume update

    // TODO add preference for min balance

    double old_conductance = Conductance(curr_cluster_cut, curr_cluster_vol);
    MoveNode<true>(u);
    double new_conductance = Conductance(curr_cluster_cut, curr_cluster_vol);

    //std::cout << "Moved node " << u << " diff " << old_conductance - new_conductance
    //          << " cut = " << curr_cluster_cut << " vol = " << curr_cluster_vol
    //          << " phi = " << new_conductance << std::endl;

    if (new_conductance < best_conductance) {
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

  return Result {
      .cut = curr_cluster_cut,
      .volume = curr_cluster_vol,
      .conductance = Conductance(curr_cluster_cut, curr_cluster_vol),
      .in_cluster = &in_cluster,
  };
}


bool SparseCutHeuristics::Compute(UnitFlow::Graph& graph, double conductance_goal, double balance_goal) {
  std::cout << "Sparse cut heuristics. conductance goal = " <<  conductance_goal << " balance goal = " << balance_goal << std::endl;
  nibble.SetGraph(graph);
  local_search.SetGraph(graph);
  auto total_volume = graph.volume();

  int prng_seed = 555;
  std::mt19937 prng(prng_seed);
  std::uniform_int_distribution<> seed_distr(0, graph.size() - 1);
  for (int r = 0; r < num_trials; ++r) {
    std::cout << "Rep " << r << " of sparse cut heuristics" << std::endl;
    Vertex seed_vertex = seed_distr(prng);
    auto nibble_cut = nibble.ComputeCut(seed_vertex);
    if (nibble_cut.conductance <= conductance_goal &&
        std::min(nibble_cut.volume, total_volume - nibble_cut.volume) >= balance_goal) {
      std::cout << "Nibble cut was balanced" << std::endl;
      for (Vertex u : nibble_cut.cut_side) in_cluster[u] = true;
      return true;
    }

    std::cout << "Try local search" << std::endl;
    auto ls_cut = local_search.Compute(nibble_cut.cut_side);
    if (ls_cut.conductance <= conductance_goal &&
        std::min(ls_cut.volume, total_volume - ls_cut.volume) >= balance_goal) {
      in_cluster = *ls_cut.in_cluster;
      return true;
    }
  }
  return false;
}

std::pair<std::vector<int>, std::vector<int>> SparseCutHeuristics::ExtractCutSides() {
  std::vector<int> a, r;
  for (int x = 0; x < in_cluster.size(); ++x) {
    if (in_cluster[x]) {
      a.push_back(x);
    } else {
      r.push_back(x);
    }
  }
  return std::make_pair(best_conductance, std::move(cutset));
}
