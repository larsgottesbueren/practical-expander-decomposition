#pragma once

#include <algorithm>
#include <iostream>
#include <numeric>
#include <queue>
#include <stack>
#include <vector>
#include <cassert>

namespace SubsetGraph {

/**
   Used to represent the number of elements still 'alive' in a vector.
 */
struct Bound {
  int middle, end;
  Bound(int middle, int end) : middle(middle), end(end) {}
  Bound(int end) : Bound(end, end) {}
};

/**
   A graph with capability to 'focus' on subsets of the graph. By reordering
   vertices and edges within adjacency lists, vertices can be temporarily
   removed from the graph.

   Notation: 'n' and 'm' are the number of vertices and edges respectively in
   the current induced subgraph.

   Operations:
   - remove(u): remove a vertex from the current subgraph.
   - subgraph(us): create a new subgraph induced by the vertices 'us'.
   - restoreRemoves(): restore all remove operations in current subgraph.
   - restoreSubgraph(): restore to the previous subgraph.
 */
template <typename V, typename E> class Graph {
private:
  /**
     Adjacency list for each vertex.
   */
  std::vector<std::vector<E>> edges;

  /**
     A stack of bounds associated with each vertex defining the number of edges
     in their adjacency list which are 'alive'.

     Current active edges for vertex 'u' are described by:
       '{edges[i] | i \in [0,edgeBounds[u].top().middle)}'
   */
  std::vector<std::stack<Bound, std::vector<Bound>>> edgeBounds;

  /**
     List of vertices in arbitrary order.
   */
  std::vector<int> vertices;

  /**
     A stack of bounds describing the current vertices which are alive:
       '{vertices[i] | i \in [0,vertexBound.top().middle}'
   */
  std::stack<Bound> vertexBound;

  /**
     List of indices such that 'vertices[vertexIndices[u]] = u'
   */
  std::vector<int> vertexIndices;

protected:
  /**
     Used to mark vertex as visited in search algorithms. Set values to 0 after
     use.
  */
  std::vector<int> visited;

public:
  /**
     Construct a graph with 'n' vertices and edges 'es'. Reverse edges are added
     automatically.

     Time complexity: O(n + m)
   */
  Graph(int n, const std::vector<E> &es)
      : edges(n), edgeBounds(n), vertices(n), vertexIndices(n), visited(n) {
    std::iota(vertices.begin(), vertices.end(), 0);
    std::iota(vertexIndices.begin(), vertexIndices.end(), 0);
    vertexBound.push({n});

    for (auto e : es) {
      auto re = e.reverse();
      e.revIdx = int(edges[e.to].size());
      re.revIdx = int(edges[e.from].size());

      edges[e.from].push_back(e);
      edges[e.to].push_back(re);
    }

    for (int u = 0; u < n; ++u)
      edgeBounds[u].push({int(edges[u].size())});
  }

  /**
     Vertex begin-iterator.

     Time complexity: O(1)
   */
  typename std::vector<V>::iterator begin() { return vertices.begin(); }

  /**
     Constant vertex begin-iterator.

     Time complexity: O(1)
   */
  typename std::vector<V>::const_iterator cbegin() const {
    return vertices.cbegin();
  }

  /**
     Vertex end-iterator.

     Time complexity: O(1)
   */
  typename std::vector<V>::iterator end() {
    return vertices.begin() + vertexBound.top().middle;
  }

  /**
     Constant vertex end-iterator.

     Time complexity: O(1)
   */
  typename std::vector<V>::const_iterator cend() const {
    return vertices.cbegin() + vertexBound.top().middle;
  }

  /**
     Begin iterator to removed vertices in current subgraph.

     Time complexity: O(1)
   */
  typename std::vector<V>::iterator beginRemoved() {
    return vertices.begin() + vertexBound.top().middle;
  }

  /**
     Constant iterator to beginning of removed vertices in current subgraph.

     Time complexity: O(1)
   */
  typename std::vector<V>::const_iterator cbeginRemoved() const {
    return vertices.cbegin() + vertexBound.top().middle;
  }

  /**
     End iterator of removed vertices in current subgraph.

     Time complexity: O(1)
   */
  typename std::vector<V>::iterator endRemoved() {
    return vertices.begin() + vertexBound.top().end;
  }

  /**
     Constant iterator to end of removed vertices in current subgraph.

     Time complexity: O(1)
   */
  typename std::vector<V>::const_iterator cendRemoved() const {
    return vertices.cbegin() + vertexBound.top().end;
  }

  /**
     Edge begin-iterator.

     Time complexity: O(1)
   */
  typename std::vector<E>::iterator beginEdge(V u) { return edges[u].begin(); }

  /**
     Constant edge begin-iterator.

     Time complexity: O(1)
   */
  typename std::vector<E>::const_iterator cbeginEdge(V u) const {
    return edges[u].cbegin();
  }

  /**
     Edge end-iterator.

     Time complexity: O(1)
   */
  typename std::vector<E>::iterator endEdge(V u) {
    return edges[u].begin() + edgeBounds[u].top().middle;
  }

  /**
     Constant edge end-iterator.

     Time complexity: O(1)
   */
  typename std::vector<E>::const_iterator cendEdge(V u) const {
    return edges[u].cbegin() + edgeBounds[u].top().middle;
  }

  /**
     Return the i'th edge in the adjacency list of vertex 'u'.
   */
  E &getEdge(V u, int idx) {
    assert(idx >= 0 && "Edge index cannot be negative.");
    assert(
        idx < edgeBounds[u].top().middle &&
        "Edge index larger than number of edges in subgraph adjacency list.");
    return edges[u][idx];
  }

  /**
     Return the i'th edge in the adjacency list of vertex 'u' as constant edge.
   */
  const E &getEdge(V u, int idx) const {
    assert(idx >= 0 && "Edge index cannot be negative.");
    assert(
        idx < edgeBounds[u].top().middle &&
        "Edge index larger than number of edges in subgraph adjacency list.");
    return edges[u][idx];
  }

  /**
     Reverse edge in graph.
   */
  E &reverse(const E &e) {
    assert(e.revIdx != -1 && "Reverse index undefined.");
    return edges[e.to][e.revIdx];
  }

  /**
     Reverse constant edge in graph.
   */
  const E &reverse(const E &e) const {
    assert(e.revIdx != -1 && "Reverse index undefined.");
    return edges[e.to][e.revIdx];
  }

  /**
     Neighbors of vertex 'u'.

     Time complexity: O(deg(u))
   */
  std::vector<V> neighbors(V u) const {
    std::vector<V> result;
    for (auto e = cbeginEdge(u); e != cendEdge(u); ++e)
      result.push_back(e->to);
    return result;
  }

  /**
     Given a subset of vertices, return the same vertices where all of their
     valid neighbors are included as well.

     Time complexity: O(vol(subset))
   */
  template <typename It>
  std::vector<V> subdivisionVertices(It subsetBegin, It subsetEnd) {
    std::vector<V> result;
    for (auto it = subsetBegin; it != subsetEnd; ++it) {
      const int u = *it;
      result.push_back(u);
      for (auto e = cbeginEdge(u); e != cendEdge(u); ++e)
        if (!visited[e->to])
          result.push_back(e->to), visited[e->to] = true;
    }

    for (auto it = subsetBegin; it != subsetEnd; ++it)
      for (auto e = cbeginEdge(*it); e != cendEdge(*it); ++e)
        visited[e->to] = false;

    return result;
  }

  /**
     Number of vertices in subgraph.

     Time complexity: O(1)
   */
  int size() const { return vertexBound.top().middle; }

  /**
     Number of vertices removed in subgraph.
   */
  int removedSize() const {
    return vertexBound.top().end - vertexBound.top().middle;
  }

  /**
     True if vertex has not been removed.
   */
  bool alive(V u) const { return vertexIndices[u] < size(); }

  /**
     Degree of vertex 'u'.

     Time complexity: O(1)
   */
  int degree(V u) const { return edgeBounds[u].top().middle; }

  /**
     Degree of vertex 'u' in entire graph.

     Time complexity: O(1)
   */
  int globalDegree(V u) const { return int(edges[u].size()); }

  /**
     Number of edges in graph.

     Time complexity: O(m)
   */
  int edgeCount() const { return volume() / 2; }

  /**
     Volume of subgraph.

     Time complexity: O(m)
   */
  int volume() const {
    int total = 0;
    for (auto it = cbegin(); it != cend(); ++it)
      total += degree(*it);
    return total;
  }

  /**
     Volume of given vertices in the current subgraph.
   */
  template <typename It> int volume(It subsetBegin, It subsetEnd) const {
    int total = 0;
    for (auto it = subsetBegin; it != subsetEnd; ++it)
      total += degree(*it);
    return total;
  }

  /**
     Volume of entire graph.

     Time complexity: O(m)
   */
  int globalVolume() const {
    int total = 0;
    for (auto it = cbegin(); it != cend(); ++it)
      total += globalDegree(*it);
    return total;
  }

  /**
     Volume of given vertices in the entire graph.
   */
  template <typename It> int globalVolume(It subsetBegin, It subsetEnd) const {
    int total = 0;
    for (auto it = subsetBegin; it != subsetEnd; ++it)
      total += globalDegree(*it);
    return total;
  }

  /**
     Find connected components using breadth first search.

     Time complexity: O(n + m)
   */
  std::vector<std::vector<V>> connectedComponents() {
    std::vector<std::vector<V>> comps;

    auto search = [&](V start) {
      std::queue<V> q;
      q.push(start);
      visited[start] = true;

      while (!q.empty()) {
        V u = q.front();
        q.pop();

        comps.back().push_back(u);

        for (auto e = cbeginEdge(u); e != cendEdge(u); ++e)
          if (!visited[e->to])
            visited[e->to] = true, q.push(e->to);
      }
    };

    for (auto it = cbegin(); it != cend(); ++it) {
      if (!visited[*it]) {
        comps.push_back({});
        search(*it);
      }
    }

    for (auto it = begin(); it != end(); ++it)
      visited[*it] = false;

    return comps;
  }

  /**
     Remove a vertex from the current subgraph.

     Time complexity: O(deg(u))
   */
  void remove(V u) {
    {
      const int fromIdx = vertexIndices[u], toIdx = --vertexBound.top().middle;
      std::swap(vertices[fromIdx], vertices[toIdx]);
      vertexIndices[u] = toIdx, vertexIndices[vertices[fromIdx]] = fromIdx;
    }

    for (auto e = beginEdge(u); e != endEdge(u); ++e) {
      const V v = e->to;
      const int fromIdx = e->revIdx, toIdx = --edgeBounds[v].top().middle;
      std::swap(edges[v][fromIdx], edges[v][toIdx]);
      reverse(edges[v][fromIdx]).revIdx = fromIdx;
      reverse(edges[v][toIdx]).revIdx = toIdx;
    }

    edgeBounds[u].top().middle = 0;
  }

  /**
     Construct a new subgraph. The given vertices must be 'alive' or 'removed'
     in the current subgraph.

     Time complexity: O(|subset| + vol(subset))
   */
  template <typename It> void subgraph(It subsetBegin, It subsetEnd) {
    vertexBound.push({0, int(std::distance(subsetBegin, subsetEnd))});

    for (auto it = subsetBegin; it != subsetEnd; ++it) {
      const int fromIdx = vertexIndices[*it],
                toIdx = vertexBound.top().middle++;
      std::swap(vertices[fromIdx], vertices[toIdx]);
      vertexIndices[vertices[fromIdx]] = fromIdx;
      vertexIndices[vertices[toIdx]] = toIdx;
    }

    assert(vertexBound.top().middle == vertexBound.top().end &&
           "Incorrect number of vertices added.");

    for (auto it = begin(); it != end(); ++it)
      visited[*it] = true;

    for (auto it = begin(); it != end(); ++it) {
      const int u = *it;
      int offset = 0;
      for (int fromIdx = 0; fromIdx < edgeBounds[u].top().end; ++fromIdx) {
        const auto &e = edges[u][fromIdx];
        if (visited[e.to]) {
          const int toIdx = offset++;
          std::swap(edges[u][fromIdx], edges[u][toIdx]);
          reverse(edges[u][fromIdx]).revIdx = fromIdx;
          reverse(edges[u][toIdx]).revIdx = toIdx;
        }
      }
      edgeBounds[u].push({offset});
    }

    for (auto it = begin(); it != end(); ++it)
      visited[*it] = false;
  }

  /**
     Restore all 'remove' operations in current subgraph.

     Time complexity: O(n)
   */
  void restoreRemoves() {
    vertexBound.top().middle = vertexBound.top().end;
    for (auto it = cbegin(); it != cend(); ++it)
      edgeBounds[*it].top().middle = edgeBounds[*it].top().end;
  }

  /**
     Restore to the previous, strictly larger, subgraph.

     Time complexity: O(n)
   */
  void restoreSubgraph() {
    for (auto it = begin(); it != end(); ++it) {
      const int u = *it;
      edgeBounds[u].pop();
      assert(!edgeBounds[u].empty() && "The top most edge bound is require to "
                                       "represent an entire adjacency list.");
    }
    vertexBound.pop();
    assert(!vertexBound.empty() &&
           "The top most vertex bound is required to represent entire graph.");
  }

  /**
     Writes the adjacency list of every active vertex.
   */
  friend std::ostream &operator<<(std::ostream &os, const Graph<V, E> &g) {
    for (auto it = g.cbegin(); it != g.cend(); ++it) {
      V u = *it;
      os << u << ":";
      for (auto e = g.cbeginEdge(u); e != g.cendEdge(u); ++e)
        os << " " << e->to;
      os << std::endl;
    }
    return os;
  }
};

}; // namespace SubsetGraph
