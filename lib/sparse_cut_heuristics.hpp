#pragma once

#include <random>
#include <vector>

#include "datastructures/priority_queue.hpp"
#include "datastructures/undirected_graph.hpp"
#include "datastructures/unit_flow.hpp"
#include "util.hpp"

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
    UnitFlow::Graph* graph;
    std::vector<double> page_rank;
    std::vector<double> residual;
    std::vector<Vertex> queue;
    std::vector<Vertex> non_zeroes;
};

class Nibble {
public:
    using Vertex = UnitFlow::Vertex;
    struct Cut {
        double cut = 0.0;
        double volume = 0.0;
        double conductance = 0.0;
        std::vector<Vertex> cut_side;
    };
    void SetGraph(UnitFlow::Graph& graph_);
    Cut ComputeCut(Vertex seed);
    void SetParams(PersonalizedPageRank::Parameters params) { ppr.params = params; }

private:
    UnitFlow::Graph* graph;
    PersonalizedPageRank ppr;
    std::vector<bool> in_cut;
    double total_vol = 0.0;
};

class LocalSearch {
public:
    using Vertex = UnitFlow::Vertex;
    struct Result {
        double cut;
        double volume;
        double conductance;
        std::vector<bool>* in_cluster;
    };

    void SetGraph(UnitFlow::Graph& graph_);
    Result Compute(const std::vector<Vertex>& seed_cluster);

private:
    void InitializeDatastructures(const std::vector<Vertex>& seed_cluster);

    template<bool update_pq>
    void MoveNode(Vertex u);

    void PQUpdate(Vertex v) {
        double gain = in_cluster[v] ? RemoveVertexConductanceGain(v) : AddVertexConductanceGain(v);
        pq.insertOrAdjustKey(v, gain);
    }

    double Conductance(double cut, double vol) const { return cut / std::min(vol, total_vol - vol); }

    double ConductanceGain(double new_cut, double new_vol) const { return Conductance(curr_cluster_cut, curr_cluster_vol) - Conductance(new_cut, new_vol); }

    double ConductanceGain(Vertex u) { return in_cluster[u] ? RemoveVertexConductanceGain(u) : AddVertexConductanceGain(u); }

    double ComputeAffinityToCluster(Vertex u) const {
        double aff = 0.0;
        for (auto edge = graph->beginEdge(u); edge != graph->endEdge(u); ++edge) {
            aff += static_cast<int>(in_cluster[edge->to]);
        }
        return aff;
    }

    bool CheckDatastructures() const {
        for (Vertex u : *graph) {
            (void) u;
            assert(ComputeAffinityToCluster(u) == affinity_to_cluster[u]);
        }
        size_t cluster_vol = 0;
        size_t cluster_cut = 0;
        for (Vertex u : *graph) {
            if (in_cluster[u])
                cluster_vol += graph->globalDegree(u);
            for (auto edge = graph->beginEdge(u); edge != graph->endEdge(u); ++edge) {
                cluster_cut += static_cast<int>(in_cluster[edge->to] != in_cluster[u]);
            }
        }
        assert(cluster_cut % 2 == 0);
        cluster_cut /= 2; // double-counted each cut edge;
        assert(cluster_vol == curr_cluster_vol);
        if (cluster_cut != curr_cluster_cut)
            VLOG(2) << V(cluster_cut) << V(curr_cluster_cut);
        assert(cluster_cut == curr_cluster_cut);
        return true;
    }

    double AddVertexConductanceGain(Vertex u) const {
        assert(!in_cluster[u]);
        double removed_cut_edges = affinity_to_cluster[u];
        double new_cut_edges = graph->degree(u) - removed_cut_edges;
        double new_cut = curr_cluster_cut + new_cut_edges - removed_cut_edges;
        double new_vol = curr_cluster_vol + graph->globalDegree(u);
        return ConductanceGain(new_cut, new_vol);
    }

    double RemoveVertexConductanceGain(Vertex u) const {
        assert(in_cluster[u]);
        double new_cut_edges = affinity_to_cluster[u];
        double removed_cut_edges = graph->degree(u) - new_cut_edges;
        double new_cut = curr_cluster_cut + new_cut_edges - removed_cut_edges;
        double new_vol = curr_cluster_vol - graph->globalDegree(u);
        return ConductanceGain(new_cut, new_vol);
    }

    bool IsMoveTabu(Vertex u) const { return last_moved_step[u] >= current_step - tabu_length; }

    mt_kahypar::ds::MaxHeap<double, Vertex> pq;
    std::vector<bool> in_cluster;
    std::vector<double> affinity_to_cluster;
    UnitFlow::Graph* graph;
    double total_vol = 0.0;
    double curr_cluster_vol = 0.0;
    double curr_cluster_cut = 0.0;

    size_t max_fruitless_moves = 300;

    std::queue<Vertex> tabu_reinsertions;
    std::vector<int> last_moved_step;
    int tabu_length = 500;
    int current_step = 0;
};

class SparseCutHeuristics {
public:
    using Vertex = UnitFlow::Vertex;

    void Allocate(UnitFlow::Graph& graph) {
        nibble.SetGraph(graph);
        local_search.SetGraph(graph);
        in_cluster.assign(graph.size(), false);
    }

    bool Compute(UnitFlow::Graph& graph, double conductance_goal, double balance_goal);

    std::pair<std::vector<int>, std::vector<int>> ExtractCutSides(UnitFlow::Graph& graph);

private:
    int num_trials = 10;
    Nibble nibble;
    LocalSearch local_search;
    std::vector<bool> in_cluster;
};
