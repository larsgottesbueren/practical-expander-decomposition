#include "ugraph.hpp"

namespace Undirected {

Edge::Edge(int from, int to) : from(from), to(to), reverse(nullptr) {}

Graph::Graph(int n) : PartitionGraph<int, Edge>(n) {}

bool Graph::addEdge(int from, int to) {
  auto p = addDirectedEdge({from, to}, true);
  if (p) {
    auto rp = addDirectedEdge({to, from}, false);
    assert(rp && "Reverse edge should exist if regular edge exists.");
    p->reverse = rp;
    rp->reverse = p;
    return true;
  } else {
    return false;
  }
}

} // namespace Undirected
