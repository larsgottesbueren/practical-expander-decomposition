#include "unit_flow.hpp"

#include <cstdint>
#include <functional>

namespace UnitFlow {

    Edge::Edge(Vertex from, Vertex to, Flow flow, Flow capacity, Flow congestion) :
        from(from), to(to), revIdx(-1), flow(flow), capacity(capacity), congestion(congestion) {}

    Edge::Edge(Vertex from, Vertex to, Flow flow, Flow capacity) : Edge(from, to, flow, capacity, 0) {}

    Edge::Edge(Vertex from, Vertex to, Flow capacity) : Edge(from, to, 0, capacity) {}

    Graph::Graph(int n, const std::vector<Edge>& es) : SubsetGraph::Graph<int, Edge>(n, es), absorbed(n), sink(n), height(n), nextEdgeIdx(n) {}

    void Graph::Push(Vertex u, Edge& e) {
        Flow delta = std::min(excess(u), e.residual());
        e.flow += delta;
        reverse(e).flow -= delta;
        absorbed[u] -= delta;
        absorbed[e.to] += delta;
    }

    bool Graph::SinglePushLowestLabel(int maxHeight) {
        const int maxH = std::min(maxHeight, size() * 2 + 1);
        size_t flow_routed = 0;
        std::vector<std::queue<Vertex>> q(maxH + 1);

        for (auto u : *this) {
            if (excess(u) > 0) {
                q[0].push(u);
            }
            flow_routed += std::min(sink[u], absorbed[u]);
        }

        size_t work = 0;
        int level = 0;
        while (level <= maxH && flow_routed <= max_flow) {
            work++;
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

            auto& e = getEdge(u, nextEdgeIdx[u]);
            assert(e.flow + reverse(e).flow == 0 && "Flow across edge and its reverse should cancel.");
            if (e.residual() > 0 && height[u] == height[e.to] + 1) {
                // Push flow across 'e'
                assert(excess(e.to) == 0 && "Pushing to vertex with non-zero excess");
                const Flow delta = std::min({ excess(u), e.residual(), (Flow) degree(e.to) });
                assert(delta > 0);
                int drain_here = std::min<int>(delta, std::max<int>(0, sink[e.to] - absorbed[e.to]));
                flow_routed += drain_here;
                e.flow += delta;
                reverse(e).flow -= delta;
                absorbed[u] -= delta;
                absorbed[e.to] += delta;

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

        VLOG(3) << V(work) << V(flow_routed);
        return flow_routed >= max_flow;
    }

    std::pair<bool, bool> Graph::computeFlow(const int maxHeight, bool warm_start) {
        if (warm_start) {
            // ForwardShiloachVishkin();
            Flow flow = Dinitz();
            if (flow >= max_flow) {
                // TODO careful with distance labels for level cut
                return std::make_pair(true, false);
            }
        }

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

    Flow Graph::Dinitz() {
        int n = size() + 1;
        size_t flow_routed = 0;
        std::vector<Vertex> frontier;
        std::vector<Vertex> stack;

        size_t work = 0;

        for (int round = 0; false || round < 2; ++round) {

            frontier.clear();
            for (Vertex u : *this) {
                height[u] = n;
                if (excess(u) > 0) {
                    frontier.push_back(u);
                    height[u] = 0;
                }
            }
            size_t num_sources = frontier.size();

            bool sink_reached = false;
            int depth = 0;
            for (size_t head = 0; head < frontier.size(); ++head) {
                Vertex u = frontier[head];
                nextEdgeIdx[u] = 0;
                for (auto& e : edgesOf(u)) {
                    if (e.residual() && height[e.to] == n) {
                        height[e.to] = height[u] + 1;
                        depth = std::max(height[e.to], depth);
                        if (!isSink(e.to)) {
                            frontier.push_back(e.to);
                        } else {
                            sink_reached = true;
                        }
                    }
                    work++;
                }
            }

            if (!sink_reached) {
                break;
            }

            for (size_t i = 0; i < num_sources; ++i) {
                stack.clear();
                Vertex source = frontier[i];
                stack.push_back(source);
                while (!stack.empty()) {
                    Vertex u = stack.back();

                    if (nextEdgeIdx[u] == degree(u)) {
                        height[stack.back()] = n;
                        stack.pop_back();
                        continue;
                    }

                    Vertex v = -1;
                    for (; nextEdgeIdx[u] < degree(u); ++nextEdgeIdx[u]) {
                        const Edge& e = getEdge(u, nextEdgeIdx[u]);
                        work++;
                        if (e.residual() && (isSink(e.to) || height[e.to] == height[u] + 1)) {
                            v = e.to;
                            break;
                        }
                    }

                    if (v != -1) {
                        if (!isSink(v)) {
                            stack.push_back(v);
                        } else {
                            Flow delta = std::numeric_limits<Flow>::max();
                            int lowest = std::numeric_limits<int>::max();
                            for (int j = 0; j < stack.size(); ++j) {
                                const Edge& e = getEdge(stack[j], nextEdgeIdx[stack[j]]);
                                if (e.residual() < delta) {
                                    delta = e.residual();
                                    lowest = j;
                                }
                            }
                            delta = std::min(delta, excess(source));
                            for (int j = 0; j < stack.size(); ++j) {
                                Vertex u = stack[j];
                                Edge& e = getEdge(u, nextEdgeIdx[u]);
                                e.flow += delta;
                                reverse(e).flow -= delta;
                                absorbed[u] -= delta;
                                absorbed[e.to] += delta;
                            }
                            stack.resize(lowest + 1);
                            flow_routed += delta;

                            if (excess(source) == 0) {
                                break;
                            }
                        }
                    }
                }
            }
        }


        for (Vertex u : *this) {
            height[u] = 0;
            nextEdgeIdx[u] = 0;
        }

        VLOG(4) << V(flow_routed) << V(max_flow) << V(work);

        return flow_routed;
    }

    void Graph::ForwardShiloachVishkin() {
        int n = size();
        std::vector<Vertex> frontier, next_frontier;
        for (Vertex u : *this) {
            height[u] = n;
            if (excess(u) > 0) {
                frontier.push_back(u);
                height[u] = 0;
            }
        }
        size_t work = 0;
        size_t sink_pushes = 0;
        size_t pushes = 0;
        size_t depth = 0;
        while (!frontier.empty()) {
            VLOG(4) << V(frontier.size());
            for (Vertex u : frontier) {
                // don't just flood the network immediately. if you can find a sink try that first
                for (auto& e : edgesOf(u)) {
                    if (excess(u) == 0) {
                        break;
                    }
                    if (e.residual() && isSink(e.to)) {
                        Push(u, e);
                        sink_pushes++;
                    }
                    work++;
                }

                for (auto& e : edgesOf(u)) {
                    if (excess(u) == 0) {
                        break;
                    }
                    if (e.residual() && height[e.to] > height[u]) {
                        Push(u, e);
                        height[e.to] = height[u] + 1;
                        next_frontier.push_back(e.to);
                        pushes++;
                    }
                    work++;
                }
            }

            depth++;
            frontier.clear();
            std::swap(next_frontier, frontier);
        }

        size_t flow_routed = 0;
        for (Vertex u : *this) {
            height[u] = 0;
            flow_routed += std::min(sink[u], absorbed[u]);
        }
        VLOG(3) << V(work) << V(flow_routed) << V(sink_pushes) << V(pushes) << V(depth);
    }

    bool Graph::StandardMaxFlow() {


        size_t flow_routed = 0;
        std::queue<Vertex> active_vertices;
        for (Vertex u : *this) {
            if (excess(u) > 0) {
                active_vertices.push(u);
            }
            flow_routed += std::min(sink[u], absorbed[u]);
        }

        const int n = size();
        const size_t global_relabel_work_threshold = 20 * n + 10 * volume();
        size_t work_since_last_global_relabel = global_relabel_work_threshold + 1;

        while (!active_vertices.empty()) {
            if (work_since_last_global_relabel > global_relabel_work_threshold) {
                work_since_last_global_relabel = 0;
                GlobalRelabel();
            }

            const Vertex u = active_vertices.front();
            active_vertices.pop();
            // discharge u
            while (excess(u) > 0) {
                if (nextEdgeIdx[u] < degree(u)) {
                    // try to push
                    auto& e = getEdge(u, nextEdgeIdx[u]);
                    if (e.residual() > 0 && height[u] > height[e.to]) {
                        if (excess(e.to) == 0) {
                            active_vertices.push(e.to);
                        }
                        Flow delta = std::min<Flow>(e.residual(), excess(u));
                        int drain_here = std::max<int>(0, sink[e.to] - absorbed[e.to]);
                        flow_routed += std::min<int>(delta, drain_here);
                        assert(delta > 0);
                        e.flow += delta;
                        reverse(e).flow -= delta;
                        absorbed[u] -= delta;
                        absorbed[e.to] += delta;
                        work_since_last_global_relabel += 3;
                    } else {
                        ++nextEdgeIdx[u];
                        work_since_last_global_relabel += 1;
                    }
                } else {
                    // relabel
                    int new_level = n + 1;
                    for (const auto& e : edgesOf(u)) {
                        if (e.residual() > 0 && height[e.to] < new_level) {
                            new_level = height[e.to];
                            height[u] = new_level + 1;
                        }
                    }
                    nextEdgeIdx[u] = 0;
                    if (new_level == n + 1) {
                        height[u] = n + 1;
                        break;
                    }
                }
            }
        }

        return false;
    }

    void Graph::GlobalRelabel() {
        std::queue<Vertex> queue;
        const int n = size();
        for (Vertex u : *this) {
            height[u] = isSink(u) ? 0 : n;
            if (isSink(u)) {
                queue.push(u);
            }
        }

        while (!queue.empty()) {
            const Vertex u = queue.front();
            queue.pop();
            const int d = height[u] + 1;
            for (const Edge& e : edgesOf(u)) {
                if (reverse(e).residual() && height[e.to] >= n) {
                    height[e.to] = d;
                    queue.push(e.to);
                }
            }
        }
    }

    std::vector<Vertex> Graph::MinCut() {
        const int n = size();
        std::vector<Vertex> source_side_cut;
        for (Vertex u : *this) {
            height[u] = n;
            if (excess(u) > 0) {
                source_side_cut.push_back(u);
                height[u] = 0;
            }
        }

        for (size_t head = 0; head < source_side_cut.size(); ++head) {
            Vertex u = source_side_cut[head];
            for (const auto& e : edgesOf(u)) {
                if (e.residual() && height[e.to] == n) {
                    source_side_cut.push_back(e.to);
                    height[e.to] = 0;
                }
            }
        }
        return source_side_cut;
    }

    std::pair<std::vector<Vertex>, std::vector<Vertex>> Graph::MinCut2() {
        const int n = size();
        std::vector<Vertex> source_side_cut;
        for (Vertex u : *this) {
            height[u] = n;
            if (excess(u) > 0) {
                source_side_cut.push_back(u);
                height[u] = 0;
            }
        }

        for (size_t head = 0; head < source_side_cut.size(); ++head) {
            Vertex u = source_side_cut[head];
            for (const auto& e : edgesOf(u)) {
                if (e.residual() && height[e.to] == n) {
                    source_side_cut.push_back(e.to);
                    height[e.to] = 0;
                }
            }
        }

        std::vector<Vertex> sink_side_cut;
        for (Vertex u : *this) {
            if (isSink(u)) {
                sink_side_cut.push_back(u);
                assert(height[u] == n);
                height[u] = 1;
            }
        }

        for (size_t head = 0; head < sink_side_cut.size(); ++head) {
            Vertex u = sink_side_cut[head];
            for (const auto& e : edgesOf(u)) {
                if (reverse(e).residual()) {
                    assert(height[e.to] == n || height[e.to] == 1);
                    if (height[e.to] == n) {
                        sink_side_cut.push_back(e.to);
                        height[e.to] = 1;
                    }
                }
            }
        }

        VLOG(4) << V(source_side_cut.size()) << V(sink_side_cut.size()) << V(size()) << "combined" << source_side_cut.size() + sink_side_cut.size();
        return std::make_pair(source_side_cut, sink_side_cut);
    }

    std::pair<std::vector<Vertex>, std::vector<Vertex>> Graph::levelCut(const int h, const double conductance_bound) {
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
                if (bestConductance < conductance_bound) {
                    break;
                }
            }
        }

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
        std::vector<decltype(beginEdge(0))> path;
        for (Vertex start : sources) {
            const int visited_label = start + 1;
            visited[start] = visited_label;
            path.push_back(beginEdge(start));
            Vertex target = -1;
            while (target == -1 && !path.empty()) {
                auto& e = path.back();
                Vertex u = e->from;
                for (; e != endEdge(u); ++e) {
                    Vertex v = e->to;
                    if (e->flow > 0 && visited[v] != visited_label) {
                        if (absorbed[v] > 0 && sink[v] > 0) {
                            target = v;
                        }
                        visited[v] = visited_label;
                        path.push_back(beginEdge(v));
                        break;
                    }
                }
                if (e == endEdge(u)) {
                    path.pop_back();
                }
            }

            if (target != -1) {
                path.pop_back(); // don't route flow on outgoing edge of target
                absorbed[target]--;
                sink[target]--;
                for (auto e : path) {
                    e->flow--;
                }
                matches.emplace_back(start, target);
            }
            path.clear();
        }

        for (auto it = cbegin(); it != cend(); ++it) {
            visited[*it] = 0;
        }

        return matches;
    }


    std::vector<std::pair<Vertex, Vertex>> Graph::matching(const std::vector<Vertex>& sources) { return matchingDfs(sources); }
} // namespace UnitFlow
