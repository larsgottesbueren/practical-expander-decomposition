#pragma once

#include <algorithm>
#include <list>
#include <queue>
#include <vector>

#include "../util.hpp"
#include "subset_graph.hpp"

namespace UnitFlow {

    using Vertex = int;
    using Flow = long long; // TODO is this really needed?

    struct Edge {
        Vertex from, to, revIdx;
        Flow flow, capacity, congestion;

        Edge(Vertex from, Vertex to, Flow flow, Flow capacity, Flow congestion);

        Edge(Vertex from, Vertex to, Flow flow, Flow capacity);
        /**
           Construct an edge between two vertices with a certain capacity and zero
           flow.
         */
        Edge(Vertex from, Vertex to, Flow capacity);

        /**
           Residual capacity. I.e. the amount of capacity left over.
         */
        Flow residual() const { return capacity - flow; }

        /**
           Construct the reverse of this edge. 'capacity' is the same but 'flow' is
           set to zero. 'revIdx' remains undefined since it is maintained by the graph
           representation.
        */
        Edge reverse() const {
            Edge e{ to, from, 0, capacity };
            return e;
        }

        /**
           Two edges are equal if all their fields agree, including reverse index.
        */
        friend bool operator==(const Edge& lhs, const Edge& rhs) {
            return lhs.from == rhs.from && lhs.to == rhs.to && lhs.revIdx == rhs.revIdx && lhs.flow == rhs.flow && lhs.capacity == rhs.capacity;
        }
    };

    /**
       Push relabel based unit flow algorithm. Based on push relabel in KACTL.
     */
    class Graph : public SubsetGraph::Graph<int, Edge> {
    private:
        /**
           The amount of flow a vertex is absorbing. In the beginning, before any flow
           has been moved, this corresponds to the source function '\Delta(v)'.
         */
        std::vector<Flow> absorbed;
        /**
           The sink capacity of a vertex, i.e. the amount of flow possible to absorb.
         */
        std::vector<Flow> sink;
        /**
           The height of a vertex.
         */
        std::vector<Vertex> height;

        /**
           For each vertex, keep track of which edge in their neighbor list they
           should consider next.
         */
        std::vector<int> nextEdgeIdx;

        bool SinglePushLowestLabel(int maxHeight);

    public:
        /**
           Construct a unit flow problem with 'n' vertices and edges 'es'.
         */
        Graph(int n, const std::vector<Edge>& es);


        /**
           Increase the amount of flow a vertex is currently absorbing.
         */
        void addSource(Vertex u, Flow amount) { absorbed[u] += amount; }

        bool isSource(Vertex u) const { return absorbed[u] > 0; }

        /**
           Increase the amount of flow a vertex is able to absorb on its own.
         */
        void addSink(Vertex u, Flow amount) { sink[u] += amount; }

        bool isSink(Vertex u) const { return sink[u] > absorbed[u]; }

        /**
           Return the excess of a node, i.e. the flow it cannot absorb.
         */
        Flow excess(Vertex u) const { return std::max((Flow) 0, absorbed[u] - sink[u]); }

        /**
           Compute max flow with push relabel and max height h. Return whether the fractional flow bound was reached (only applies if that setting
           is enabled) and whether there are nodes with excess flow.
         */
        std::pair<bool, bool> computeFlow(const int maxHeight);

        bool StandardMaxFlow();

        void GlobalRelabel();

        /**
           Compute a level cut. See Saranurak and Wang A.1.

           Precondition: A flow has been computed.
         */
        std::pair<std::vector<Vertex>, std::vector<Vertex>> levelCut(const int maxHeight, const double conductance_bound = -1.0);

        /**
           Set all flow, sinks and source capacities to 0.
         */
        void reset();

        double excess_fraction = std::numeric_limits<double>::max();

    private:
        std::vector<std::pair<Vertex, Vertex>> matchingDfs(const std::vector<Vertex>& sources);


    public:
        std::vector<std::pair<Vertex, Vertex>> matching(const std::vector<Vertex>& sources);
    };
} // namespace UnitFlow
