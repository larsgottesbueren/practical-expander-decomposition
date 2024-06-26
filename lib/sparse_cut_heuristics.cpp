#include "sparse_cut_heuristics.hpp"
#include "metis.h"

void PersonalizedPageRank::Compute(Vertex seed) {
    // clear old data
    for (Vertex u : non_zeroes) {
        residual[u] = 0.0;
        page_rank[u] = 0.0;
    }
    non_zeroes.clear();

    // add new seed
    queue.push_back(seed);
    non_zeroes.push_back(seed);
    residual[seed] = 1.0;

    // push loop
    while (!queue.empty()) {
        const Vertex u = queue.front();
        queue.pop_front();
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
    for (Vertex u : non_zeroes) {
        if (page_rank[u] > 0.0) {
            result.emplace_back(page_rank[u], u);
        }
    }
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
    curr_cluster_cut += multiplier * (graph->degree(u) - 2 * affinity_to_cluster[u]);
    for (auto e = graph->beginEdge(u); e != graph->endEdge(u); ++e) {
        Vertex v = e->to;
        assert(v != u);
        affinity_to_cluster[v] += multiplier;
        if constexpr (update_pq) {
            PQUpdate(v);
        }
    }
    if (static_cast<size_t>(graph->size()) != in_cluster.size()) {
        assert(CheckDatastructures());
    }
}

void LocalSearch::InitializeDatastructures(const std::vector<LocalSearch::Vertex>& seed_cluster) {
    current_step = 0;
    in_cluster.assign(in_cluster.size(), false);
    pq.clear();
    while (!tabu_reinsertions.empty())
        tabu_reinsertions.pop();

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
        curr_cluster_vol += graph->globalDegree(u);
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

void LocalSearch::ResetDatastructures() {
    for (Vertex u : *graph) {
        affinity_to_cluster[u] = 0;
        last_moved_step[u] = std::numeric_limits<int>::min();
        in_cluster[u] = false;
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

void BalancedPartitioner::Allocate(size_t num_nodes) {
    node_id_remap.resize(num_nodes);
    partition.resize(num_nodes);
    partition2.resize(num_nodes);
}

BalancedPartitioner::Result BalancedPartitioner::Compute(UnitFlow::Graph& graph) {
    // remap node IDs
    Vertex remapped_id = 0;
    for (Vertex u : graph) {
        node_id_remap[u] = remapped_id++;
    }

    // fill CSR
    csr.adj.clear();
    csr.xadj.clear();
    csr.xadj.push_back(0);
    csr.vwgt.clear();
    for (Vertex u : graph) {
        for (auto e = graph.beginEdge(u); e != graph.endEdge(u); ++e) {
            csr.adj.push_back(node_id_remap[e->to]);
        }
        csr.xadj.push_back(csr.adj.size());
        csr.vwgt.push_back(graph.globalDegree(u));
    }

    // call Metis
    int32_t nvtxs = csr.vwgt.size();
    int32_t ncon = 1;
    int32_t* vsize = nullptr;
    int32_t* adjwgt = nullptr;
    int32_t nparts = 2;
    std::array<float, 2> tpwgts = { 0.5f, 0.5f };
    float ubvec = 1.2f;
    int32_t objval = 0;
    int32_t options[METIS_NOPTIONS];
    METIS_SetDefaultOptions(options);
    options[METIS_OPTION_NO2HOP] = 0;
    METIS_PartGraphRecursive(&nvtxs, &ncon, csr.xadj.data(), csr.adj.data(), csr.vwgt.data(), vsize, adjwgt, &nparts, tpwgts.data(), &ubvec, options, &objval,
                             partition.data());

    // reset node id remap, compute volume, translate partition assignment
    size_t i = 0;
    double vol1 = 0, vol2 = 0;
    for (Vertex u : graph) {
        partition2[u] = partition[i];
        vol1 += csr.vwgt[i] * partition[i];
        vol2 += csr.vwgt[i] * (1 - partition[i]);
        ++i;
        node_id_remap[u] = -1;
    }

    return Result{ .cut = static_cast<double>(objval),
                   .volume1 = vol1,
                   .volume2 = vol2,
                   .conductance = static_cast<double>(objval) / std::min(vol1, vol2),
                   .partition = &partition2 };
}


bool SparseCutHeuristics::Compute(UnitFlow::Graph& graph, double conductance_goal, double balance_goal, bool use_balanced_partitions) {
    VLOG(2) << "Sparse cut heuristics. conductance goal = " << conductance_goal << " balance goal = " << balance_goal;

    if (use_balanced_partitions) {
        auto bp_cut = balanced_partitioner.Compute(graph);
        VLOG(3) << V(bp_cut.cut) << V(bp_cut.conductance) << V(bp_cut.volume1) << V(bp_cut.volume2);
        if (bp_cut.conductance < conductance_goal && std::min(bp_cut.volume1, bp_cut.volume2) >= balance_goal) {
            for (Vertex u : graph) {
                in_cluster[u] = (*bp_cut.partition)[u] == 0;
            }
            //  TODO too stronk
            return true;
        }
    }


    double best_conductance = std::numeric_limits<double>::max();
    nibble.SetGraph(graph);
    local_search.SetGraph(graph);

    auto total_volume = graph.globalVolume();
    int prng_seed = 555;
    std::mt19937 prng(prng_seed);
    std::uniform_int_distribution<> seed_distr(0, graph.size() - 1);
    for (int r = 0; r < num_trials; ++r) {
        // TODO try different PPR parameters

        const int seed_vertex_index = seed_distr(prng);
        const Vertex seed_vertex = *(graph.cbegin() + seed_vertex_index);
        const auto nibble_cut = nibble.ComputeCut(seed_vertex);
        VLOG(3) << "Nibble cut phi " << nibble_cut.conductance << V(nibble_cut.cut) << V(nibble_cut.volume);
        if (nibble_cut.conductance < best_conductance && std::min(nibble_cut.volume, total_volume - nibble_cut.volume) >= balance_goal) {
            in_cluster.assign(in_cluster.size(), false);
            for (Vertex u : nibble_cut.cut_side) {
                in_cluster[u] = true;
            }
            best_conductance = nibble_cut.conductance;
        }

        auto ls_cut = local_search.Compute(nibble_cut.cut_side);
        VLOG(3) << "Local search cut phi " << ls_cut.conductance << V(ls_cut.cut) << V(ls_cut.volume);
        if (ls_cut.conductance < best_conductance && std::min(ls_cut.volume, total_volume - ls_cut.volume) >= balance_goal) {
            in_cluster = *ls_cut.in_cluster;
            for (Vertex u : graph) {
                in_cluster[u] = (*ls_cut.in_cluster)[u];
            }
            best_conductance = ls_cut.conductance;
        }
        local_search.ResetDatastructures();
    }
    VLOG(2) << "Sparsest cut " << best_conductance << "/" << conductance_goal;
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
    return std::make_pair(std::move(a), std::move(r));
}
