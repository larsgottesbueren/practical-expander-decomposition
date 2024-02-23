#include "sparse_cut_heuristics.hpp"

void PersonalizedPageRank::Compute(Vertex seed) {
    // clear old queue
    for (Vertex u : non_zeroes) {
        residual[u] = 0.0;
        page_rank[u] = 0.0;
    }
    non_zeroes.clear();
    queue.clear();  // TODO try deque instead of vector in performance tuning

    // add new seed
    queue.push_back(seed);
    residual[seed] = 1.0;

    // push loop
    for (size_t i = 0; i < queue.size(); i++) {
        const Vertex u = queue[i];
        const double res_u = residual[u];
        const double mass_preserved = (1.0 - params.alpha) * res_u / 2;
        const double mass_pushed_to_neighbors = mass_preserved / graph->degree(u);

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
    total_vol = graph->globalVolume();
}

Nibble::Cut Nibble::ComputeCut(Vertex seed) {
    ppr.Compute(seed);
    auto ppr_distr = ppr.ExtractSparsePageRankValues();
    for (auto& pru : ppr_distr) {
        pru.pr = pru.pr / graph->globalDegree(pru.u);
    }
    std::sort(ppr_distr.begin(), ppr_distr.end(), [](const auto& l, const auto& r) { return l.pr > r.pr; });

    double cut = 0;
    double vol = 0;
    double best_conductance = std::numeric_limits<double>::max();
    int best_cut_index = -1;

    for (int i = 0; i < int(ppr_distr.size()); ++i) {
        const Vertex u = ppr_distr[i].u;
        vol += graph->globalDegree(u);

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
        result.volume += graph->globalDegree(u);
    }
    result.cut = best_conductance * std::min(result.volume, total_vol - result.volume);

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
    total_vol = graph->globalVolume();
}

template<bool update_pq>
void LocalSearch::MoveNode(Vertex u) {
    int multiplier = in_cluster[u] ? -1 : 1;
    in_cluster[u] = !in_cluster[u];
    curr_cluster_vol += multiplier * graph->globalDegree(u);
    curr_cluster_cut += multiplier * (graph->globalDegree(u) - 2 * affinity_to_cluster[u]);
    for (auto e = graph->beginEdge(u); e != graph->endEdge(u); ++e) {
        Vertex v = e->to;
        assert(v != u);
        affinity_to_cluster[v] += multiplier;
        if constexpr (update_pq) {
            PQUpdate(v);
        }
    }
    // assert(CheckDatastructures());
}


LocalSearch::Result LocalSearch::Compute2(const std::vector<LocalSearch::Vertex>& seed_cluster) {
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

        const double prev_conductance = Conductance(curr_cluster_cut, curr_cluster_vol);
        MoveNode<false>(best_move_node); // changes in_cluster --> capture before
        const double current_conductance = Conductance(curr_cluster_cut, curr_cluster_vol);
        const double recalculated_gain = prev_conductance - current_conductance;
        assert(DoubleEquals(recalculated_gain, best_gain));
        ++total_moves;

        last_moved_step[best_move_node] = current_step++;
        if (current_step == std::numeric_limits<int>::max()) {
            last_moved_step.assign(last_moved_step.size(), std::numeric_limits<int>::min());
            current_step = 0;
        }

        fruitless_moves.push_back(best_move_node);
        if (current_conductance < best_conductance) {
            best_conductance = current_conductance;
            fruitless_moves.clear();
        }
    }

    for (Vertex u : fruitless_moves) {
        MoveNode<false>(u);
    }
    VLOG(3) << V(best_conductance) << V(Conductance(curr_cluster_cut, curr_cluster_vol)) << V(total_moves);

    return Result{
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
    while (!tabu_reinsertions.empty())
        tabu_reinsertions.pop();
    last_moved_step.assign(last_moved_step.size(), std::numeric_limits<int>::min());

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

LocalSearch::Result LocalSearch::Compute(const std::vector<LocalSearch::Vertex>& seed_cluster) {
    InitializeDatastructures(seed_cluster);

    std::vector<Vertex> fruitless_moves;
    double best_conductance = Conductance(curr_cluster_cut, curr_cluster_vol);

    while (!pq.empty() && fruitless_moves.size() < max_fruitless_moves) {
        // TODO add preference for min balance
        Vertex u = pq.top();
        pq.deleteTop();
        if (IsMoveTabu(u)) {
            tabu_reinsertions.push(u);
            continue;
        }
        const double conductance_gain = ConductanceGain(u);
        if (conductance_gain < pq.topKey()) {
            // freshen up the PQ and try again
            pq.insertOrAdjustKey(u, conductance_gain);
            continue;
        }

        double old_conductance = Conductance(curr_cluster_cut, curr_cluster_vol);
        MoveNode<true>(u);
        const double new_conductance = Conductance(curr_cluster_cut, curr_cluster_vol);

        last_moved_step[u] = current_step++;
        while (!tabu_reinsertions.empty() && !IsMoveTabu(tabu_reinsertions.front())) {
            Vertex v = tabu_reinsertions.front();
            tabu_reinsertions.pop();
            pq.insertOrAdjustKey(v, ConductanceGain(v));
        }

        if (new_conductance < best_conductance) {
            fruitless_moves.clear();
            best_conductance = new_conductance;
        } else {
            fruitless_moves.push_back(u);
        }
    }

    // revert fruitless moves
    for (const Vertex u : fruitless_moves) {
        MoveNode<false>(u);
    }

    return Result{
        .cut = curr_cluster_cut,
        .volume = curr_cluster_vol,
        .conductance = Conductance(curr_cluster_cut, curr_cluster_vol),
        .in_cluster = &in_cluster,
    };
}


bool SparseCutHeuristics::Compute(UnitFlow::Graph& graph, double conductance_goal, double balance_goal) {
    VLOG(1) << "Sparse cut heuristics. conductance goal = " << conductance_goal << " balance goal = " << balance_goal;
    nibble.SetGraph(graph);
    local_search.SetGraph(graph);
    auto total_volume = graph.volume();
    VLOG(3) << V(total_volume);

    double best_conductance = std::numeric_limits<double>::max();

    int prng_seed = 555;
    std::mt19937 prng(prng_seed);
    std::uniform_int_distribution<> seed_distr(0, graph.size() - 1);
    for (int r = 0; r < num_trials; ++r) {
        // TODO try different PPR parameters

        const int seed_vertex_index = seed_distr(prng);
        const Vertex seed_vertex = *(graph.cbegin() + seed_vertex_index);
        const auto nibble_cut = nibble.ComputeCut(seed_vertex);
        VLOG(2) << "Nibble cut phi " << nibble_cut.conductance << V(nibble_cut.cut) << V(nibble_cut.volume);
        if (nibble_cut.conductance < best_conductance && std::min(nibble_cut.volume, total_volume - nibble_cut.volume) >= balance_goal) {
            in_cluster.assign(in_cluster.size(), false);
            for (Vertex u : nibble_cut.cut_side) {
                in_cluster[u] = true;
            }
            best_conductance = nibble_cut.conductance;
        }

        auto ls_cut = local_search.Compute(nibble_cut.cut_side);
        VLOG(2) << "Local search cut phi " << ls_cut.conductance << V(ls_cut.cut) << V(ls_cut.volume);
        if (ls_cut.conductance < best_conductance && std::min(ls_cut.volume, total_volume - ls_cut.volume) >= balance_goal) {
            in_cluster = *ls_cut.in_cluster;
            best_conductance = ls_cut.conductance;
        }
    }
    VLOG(1) << "Sparsest cut " << best_conductance << "/" << conductance_goal;
    return best_conductance <= conductance_goal;
}

std::pair<std::vector<int>, std::vector<int>> SparseCutHeuristics::ExtractCutSides(UnitFlow::Graph& graph) {
    std::vector<int> a, r;
    for (Vertex u : graph) {
        if (in_cluster[u]) {
            a.push_back(u);
        } else {
            r.push_back(u);
        }
    }
    VLOG(2) << V(a.size()) << V(r.size());
    return std::make_pair(std::move(a), std::move(r));
}
