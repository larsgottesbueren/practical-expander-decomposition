#include "sparse_cut_heuristics.hpp"

void PersonalizedPageRank::Compute(Vertex seed) {

  //residual.assign(residual.size(), 0.0);
  //page_rank.assign(page_rank.size(), 0.0);

  // clear old queue
  for (Vertex u : non_zeroes) {
    residual[u] = 0.0;
    page_rank[u] = 0.0;
  }
  non_zeroes.clear();
  queue.clear();

  for (size_t i = 0; i < residual.size(); ++i) { if (residual[i] != 0.0 || page_rank[i] != 0.0) throw std::runtime_error("residual or pagerank vector not clean"); }

  // add new seed
  queue.push_back(seed);
  residual[seed] = 1.0;

  // push loop
  for (size_t i = 0; i < queue.size(); i++) {
    const Vertex u = queue[i];
    if (!graph->alive(u)) {
      VLOG(1) << u << seed;
      throw std::runtime_error("Node unalive");
    }
    const double res_u = residual[u];
    const double mass_preserved = (1.0-params.alpha)*res_u/2;
    const double mass_pushed_to_neighbors = mass_preserved / graph->degree(u);  // TODO beware. do we need the volume in the surrounding graph?

    for (auto e = graph->beginEdge(u); e != graph->endEdge(u); ++e) {
      const Vertex v = e->to;
      const double insert_threshold = params.epsilon * graph->degree(v);
      if (residual[v] == 0.0) {
        non_zeroes.push_back(v);
      }
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
    if (!graph->alive(u)) { throw std::runtime_error("Node unalive -- extract"); }
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
  for (size_t i = 0; i < in_cut.size(); ++i) {
    if (in_cut[i]) throw std::runtime_error("in_cut in Nibble unclean");
  }
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

  for (const auto& x : ppr_distr) {
    in_cut[x.u] = false;
  }

  Nibble::Cut result;
  result.conductance = best_conductance;
  for (int i = 0; i <= best_cut_index; ++i) {
    Vertex u = ppr_distr[i].u;
    result.cut_side.push_back(u);
    result.volume += graph->degree(u);
  }
  result.cut = best_conductance * std::min(result.volume, total_vol - result.volume);

  VLOG(3) << "len(PPR) = " << ppr_distr.size() << " best cut index = " << best_cut_index
            << " cut = " << result.cut << " vol = " << result.volume
            << " conductance = " << result.conductance;

  return result;
}

void LocalSearch::SetGraph(UnitFlow::Graph& graph_) {
  graph = &graph_;
  if (affinity_to_cluster.size() < size_t(graph->size())) {
    affinity_to_cluster.resize(graph->size(), 0);
    in_cluster.resize(graph->size(), false);
    pq.resize(graph->size());
    last_moved_step.resize(graph->size(), std::numeric_limits<int>::min());
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
    assert(v != u);
    affinity_to_cluster[v] += multiplier;
    if constexpr (update_pq) { PQUpdate(v); }
  }
  assert(CheckDatastructures());
}


LocalSearch::Result LocalSearch::Compute2(std::vector<LocalSearch::Vertex>& seed_cluster) {
  // clean up old datastructures
  affinity_to_cluster.assign(affinity_to_cluster.size(), 0);
  in_cluster.assign(in_cluster.size(), false);
  last_moved_step.assign(last_moved_step.size(), std::numeric_limits<int>::min());

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

  std::vector<Vertex> fruitless_moves;
  double best_conductance = Conductance(curr_cluster_cut, curr_cluster_vol);

#ifndef NDEBUG
  std::cout << "Take snapshot" << std::endl;
  auto in_cluster_snapshot = in_cluster;
  auto affinity_snapshot = affinity_to_cluster;
#endif

  size_t total_moves = 0;
  while (fruitless_moves.size() < max_fruitless_moves) {
    Vertex best_move_node = -1;
    double best_gain = std::numeric_limits<double>::lowest();
    for (Vertex u : *graph) {
      if (last_moved_step[u] >= current_step - tabu_length) {
        continue;
      }
      double gain = ConductanceGain(u);
      if (gain > best_gain) {
        best_gain = gain;
        best_move_node = u;
      }
    }

    if (best_move_node == -1) {
      break;
    }

    double prev_conductance = Conductance(curr_cluster_cut, curr_cluster_vol);
    MoveNode<false>(best_move_node);  // changes in_cluster --> capture before
    double current_conductance = Conductance(curr_cluster_cut, curr_cluster_vol);
    double recalculated_gain = prev_conductance - current_conductance;
    assert(DoubleEquals(recalculated_gain, best_gain));
    ++total_moves;

    last_moved_step[best_move_node] = current_step++;
    if (current_step == std::numeric_limits<int>::max()) {
      last_moved_step.assign(last_moved_step.size(), std::numeric_limits<int>::min());
      current_step = 0;
    }

    VLOG(4)    << "Moved node " << best_move_node << " diff " << recalculated_gain
                  << " cut = " << curr_cluster_cut << " vol = " << curr_cluster_vol
                  << " phi = " << current_conductance << "step =" << total_moves;

    fruitless_moves.push_back(best_move_node);
    if (current_conductance < best_conductance) {
      best_conductance = current_conductance;
      fruitless_moves.clear();
#ifndef NDEBUG
      in_cluster_snapshot = in_cluster;
      affinity_snapshot = affinity_to_cluster;
#endif
    }
  }

  for (Vertex u : fruitless_moves) {
    MoveNode<false>(u);
  }
  VLOG(3) << V(best_conductance) << V(Conductance(curr_cluster_cut, curr_cluster_vol)) << V(total_moves);
  assert(in_cluster_snapshot == in_cluster);
  assert(affinity_snapshot == affinity_to_cluster);

  return Result {
      .cut = curr_cluster_cut,
      .volume = curr_cluster_vol,
      .conductance = Conductance(curr_cluster_cut, curr_cluster_vol),
      .in_cluster = &in_cluster,
  };
}

void LocalSearch::InitializeDatastructures(const std::vector<LocalSearch::Vertex>& seed_cluster) {
  // TODO make the runtime of this sensitive again to the size of the local search (or at least the current subgraph size)
#if false
  // clean up old datastructures
  for (size_t i = 0; i < pq.size(); ++i) {
    Vertex u = pq.at(i);
    affinity_to_cluster[u] = 0;
    in_cluster[u] = false;
  }
  pq.clear();
  while (!tabu_reinsertions.empty()) {
    Vertex u = tabu_reinsertions.front();
    affinity_to_cluster[u] = 0;
    in_cluster[u] = false;
    tabu_reinsertions.pop();
  }
#endif

  affinity_to_cluster.assign(affinity_to_cluster.size(), 0);
  in_cluster.assign(in_cluster.size(), false);
  pq.clear();
  while (!tabu_reinsertions.empty()) tabu_reinsertions.pop();
  last_moved_step.assign(last_moved_step.size(), std::numeric_limits<int>::min());

  if (!std::ranges::all_of(affinity_to_cluster, [](const auto& x) { return x == 0; })
      || !std::ranges::all_of(in_cluster, [](const bool x) { return !x; })) {
    throw std::runtime_error("data structures not cleaned");
  }

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
}

LocalSearch::Result LocalSearch::Compute(std::vector<LocalSearch::Vertex>& seed_cluster) {
  InitializeDatastructures(seed_cluster);

  std::vector<Vertex> fruitless_moves;
  double best_conductance = Conductance(curr_cluster_cut, curr_cluster_vol);
  VLOG(3)   << "Vol = " << curr_cluster_vol << " Cut = " << curr_cluster_cut << " Conductance = "
                << best_conductance << " PQ size " << pq.size();

  while (!pq.empty() && fruitless_moves.size() < max_fruitless_moves) {
    // TODO add preference for min balance
    Vertex u = pq.top();
    pq.deleteTop();
    if (IsMoveTabu(u)) {
      tabu_reinsertions.push(u);
      continue;
    }
    double conductance_gain = ConductanceGain(u);
    if (conductance_gain < pq.topKey()) {
      // freshen up the PQ and try again
      pq.insertOrAdjustKey(u, conductance_gain);
      continue;
    }

    double old_conductance = Conductance(curr_cluster_cut, curr_cluster_vol);
    MoveNode<true>(u);
    double new_conductance = Conductance(curr_cluster_cut, curr_cluster_vol);

    last_moved_step[u] = current_step++;
    while (!tabu_reinsertions.empty() && !IsMoveTabu(tabu_reinsertions.front())) {
      Vertex v = tabu_reinsertions.front();
      tabu_reinsertions.pop();
      pq.insertOrAdjustKey(v, ConductanceGain(v));
    }

    VLOG(3) << "Moved node" << u << "diff" << old_conductance - new_conductance
               << "predicted gain" << conductance_gain
               << "cut =" << curr_cluster_cut << "vol =" << curr_cluster_vol
               << "phi =" << new_conductance;

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
  VLOG(1) << "Sparse cut heuristics. conductance goal = " <<  conductance_goal << " balance goal = " << balance_goal;
  nibble.SetGraph(graph);
  local_search.SetGraph(graph);
  auto total_volume = graph.volume();

  double best_conductance = std::numeric_limits<double>::max();

  int prng_seed = 555;
  std::mt19937 prng(prng_seed);
  std::uniform_int_distribution<> seed_distr(0, graph.size() - 1);
  for (int r = 0; r < num_trials; ++r) {
    // TODO try different PPR parameters

    int seed_vertex_index = seed_distr(prng);
    Vertex seed_vertex = *(graph.cbegin() + seed_vertex_index);
    auto nibble_cut = nibble.ComputeCut(seed_vertex);
    VLOG(2) << "Nibble cut phi " << nibble_cut.conductance;
    if (nibble_cut.conductance <= conductance_goal && nibble_cut.conductance < best_conductance &&
        std::min(nibble_cut.volume, total_volume - nibble_cut.volume) >= balance_goal) {
      VLOG(3) << "Nibble cut was balanced";
      in_cluster.assign(in_cluster.size(), false);
      for (Vertex u : nibble_cut.cut_side) {
        in_cluster[u] = true;
      }
      best_conductance = nibble_cut.conductance;
    }

    auto ls_cut = local_search.Compute(nibble_cut.cut_side);
    VLOG(2) << "Local search cut phi " << ls_cut.conductance;
    if (ls_cut.conductance <= conductance_goal && ls_cut.conductance < best_conductance &&
        std::min(ls_cut.volume, total_volume - ls_cut.volume) >= balance_goal) {
      in_cluster = *ls_cut.in_cluster;
      best_conductance = ls_cut.conductance;
    }
  }
  VLOG(1) << "Sparsest cut " << best_conductance << "/" << conductance_goal;
  return best_conductance <= conductance_goal;
}

std::pair<std::vector<int>, std::vector<int>> SparseCutHeuristics::ExtractCutSides() {
  std::vector<int> a, r;
  for (size_t x = 0; x < in_cluster.size(); ++x) {
    if (in_cluster[x]) {
      a.push_back(x);
    } else {
      r.push_back(x);
    }
  }
  return std::make_pair(std::move(a), std::move(r));
}
