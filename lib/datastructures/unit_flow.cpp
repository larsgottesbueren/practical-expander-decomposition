#include "unit_flow.hpp"

#include <cstdint>
#include <functional>

namespace UnitFlow {

    Edge::Edge(Vertex from, Vertex to, Flow flow, Flow capacity, Flow congestion) :
        from(from), to(to), revIdx(-1), flow(flow), capacity(capacity), congestion(congestion) {}

    Edge::Edge(Vertex from, Vertex to, Flow flow, Flow capacity) : Edge(from, to, flow, capacity, 0) {}

    Edge::Edge(Vertex from, Vertex to, Flow capacity) : Edge(from, to, 0, capacity) {}

    Graph::Graph(int n, const std::vector<Edge>& es) : SubsetGraph::Graph<int, Edge>(n, es), absorbed(n), sink(n), height(n), nextEdgeIdx(n) {}

    bool Graph::SinglePushLowestLabel(int maxHeight) {
        const int maxH = std::min(maxHeight, size() * 2 + 1);
        size_t flow_routed = 0;
        std::vector<std::queue<Vertex>> q(maxH + 1);

        for (auto u : *this) {
            if (excess(u) > 0) {
                q[0].push(u);
            }
        }

        int level = 0;

        int steps = 0;
        int pushes = 0;

        while (level <= maxH) {
            if (q[level].empty()) {
                level++;
                continue;
            }

            const int u = q[level].front();
            if (degree(u) == 0) {
                q[level].pop();
                continue;
            }
            assert(excess(u) > 0 && "Vertex popped from queue should have excess flow.");

            steps++;

            auto& e = getEdge(u, nextEdgeIdx[u]);
            assert(e.flow + reverse(e).flow == 0 && "Flow across edge and its reverse should cancel.");
            if (e.residual() > 0 && height[u] == height[e.to] + 1) {
                // Push flow across 'e'
                assert(excess(e.to) == 0 && "Pushing to vertex with non-zero excess");
                const Flow delta = std::min({ excess(u), e.residual(), (Flow) degree(e.to) });
                assert(delta > 0);
                flow_pushed_since += delta;

                e.flow += delta;
                reverse(e).flow -= delta;
                absorbed[u] -= delta;
                absorbed[e.to] += delta;

                pushes++;

                if (sink[e.to] > 0) {
                    flow_routed += delta;
                    if (flow_routed >= excess_fraction) {
                        return true;
                    }
                }

                assert(excess(u) >= 0 && "Excess after pushing cannot be negative");
                if (height[u] >= maxH || excess(u) == 0) {
                    q[level].pop();
                }

                if (height[e.to] < maxH && excess(e.to) > 0) {
                    q[height[e.to]].push(e.to);
                    level = std::min(level, height[e.to]);
                    nextEdgeIdx[e.to] = 0;
                }
            } else if (nextEdgeIdx[u] == degree(u) - 1) {
                // all edges have been tried, relabel
                q[level].pop();
                height[u]++;
                nextEdgeIdx[u] = 0;
                if (height[u] < maxH) {
                    q[height[u]].push(u);
                }
            } else {
                nextEdgeIdx[u]++;
            }
        }

        return false;
    }

    std::pair<bool, bool> Graph::compute(const int maxHeight) {
        bool reached_flow_fraction = SinglePushLowestLabel(maxHeight);

        for (auto u : *this) {
            for (auto e = beginEdge(u); e != endEdge(u); ++e) {
                if (e->flow > 0) {
                    e->congestion += e->flow;
                }
            }
        }

        bool has_excess = false;
        for (auto u : *this) {
            if (excess(u) > 0) {
                has_excess = true;
                break;
            }
        }
        return std::make_pair(reached_flow_fraction, has_excess);
    }

    std::pair<std::vector<Vertex>, std::vector<Vertex>> Graph::levelCut(const int h) {
        std::vector<std::vector<Vertex>> levels(h + 1);
        for (auto u : *this)
            levels[height[u]].push_back(u);

        int volume = 0;
        const int total_volume = this->globalVolume(); // TODO volume() or globalVolume()? Note: globalVolume() only looks at nodes in the active subgraph
        double bestConductance = 1.0;
        int bestLevel = h + 1;
        int cut = 0;
        for (int level = h; level > 0; --level) {
            for (auto u : levels[level]) {
                volume += globalDegree(u); // TODO degree() or globalDegree()?
                for (auto e = beginEdge(u); e != endEdge(u); ++e) {
                    cut += signum(level - height[e->to]);
                }
            }
            double conductance = double(cut) / double(std::min(volume, total_volume - volume));
            if (cut > 0 && conductance < bestConductance) {
                bestConductance = conductance, bestLevel = level;
            }
        }

        VLOG(4) << V(bestConductance);

        std::vector<int> left, right;
        if (bestLevel != h + 1) {
            for (int level = h; level >= bestLevel; --level) {
                for (auto u : levels[level]) {
                    left.push_back(u);
                }
            }
            for (int level = 0; level < bestLevel; ++level) {
                for (auto u : levels[level]) {
                    right.push_back(u);
                }
            }
        }
        return std::make_pair(left, right);
    }

    void Graph::reset() {
        for (auto u : *this) {
            for (auto e = beginEdge(u); e != endEdge(u); ++e) {
                e->flow = 0;
            }
            absorbed[u] = 0;
            sink[u] = 0;
            height[u] = 0;
            nextEdgeIdx[u] = 0;
        }
    }

    std::vector<std::pair<Vertex, Vertex>> Graph::matchingDfs(const std::vector<Vertex>& sources) {
        std::vector<std::pair<Vertex, Vertex>> matches;

        auto search = [&](Vertex start) {
            std::vector<Edge*> path;
            std::function<Vertex(Vertex)> dfs = [&](Vertex u) {
                visited[u] = start + 1;

                if (absorbed[u] > 0 && sink[u] > 0) {
                    absorbed[u]--, sink[u]--;
                    return u;
                }

                for (auto e = beginEdge(u); e != endEdge(u); ++e) {
                    int v = e->to;
                    if (e->flow <= 0 || visited[v] == start + 1)
                        continue;

                    path.push_back(&*e);
                    int m = dfs(v);
                    if (m != -1)
                        return m;
                    path.pop_back();
                }

                return -1;
            };

            int m = dfs(start);
            if (m != -1)
                for (auto e : path)
                    e->flow--;
            return m;
        };

        for (auto u : sources) {
            int m = search(u);
            if (m != -1)
                matches.push_back({ u, m });
        }

        for (auto it = cbegin(); it != cend(); ++it)
            visited[*it] = false;

        return matches;
    }


    std::vector<std::pair<Vertex, Vertex>> Graph::matching(const std::vector<Vertex>& sources) { return matchingDfs(sources); }
} // namespace UnitFlow
