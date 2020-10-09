#pragma once

#include <vector>

#include "splay_tree.hpp"

/**
   Link-cut implementation with support for min-queries and updates on paths.
   Based on several sources:
   - MIT Erik Demaine lecture 'Dynamic Graphs I'
   (https://courses.csail.mit.edu/6.851/spring12/)
   - http://planarity.org Optimization Algorithms for Planar Graphs: Chapter 17:
   Splay trees and link-cut trees
   - http://people.seas.harvard.edu/~cs224/spring17/lec/lec26.pdf
 */

namespace LinkCut {

using Vertex = int;

class Forest {
private:
  std::vector<SplayTree::Vertex> vertices;

  /**
     Access a vertex 'u'. This does two things. Firstly 'u' is splayed within
     it's aux-tree. Secondly, iteratively move 'u' to the top tree of
     aux-trees. This guarantees that 'u' to the root is a preferred path.
   */
  void access(Vertex u);

public:
  /**
     Construct a forest with 'n' nodes.
   */
  Forest(int n);

  /**
     Add a directed edge '(u,v)' with a certain weight. This makes 'v' the
     parent of 'u' in the rooted forest.

     Precondition: 'u' does not have a parent.
   */
  void link(Vertex u, Vertex v, int weight);

  /**
     Cut the edge '(u,p(u))' where 'p(u)' is the parent of 'u'. Returns 'p(u)'.

     Precondition: 'p(u)' must exist.
   */
  Vertex cut(Vertex u);

  /**
     Return true if 'u' and 'v' are part of the same tree. This is done by
     calling 'findRoot' twice.
   */
  bool connected(Vertex u, Vertex v);

  /**
     Find the root of 'u'.
   */
  Vertex findRoot(Vertex u);

  /**
     Find minimum value along path from 'u' to root. Return pair
     '(val,vertex)'. If several vertices have the same minimum value, the vertex
     closest to the root is returned.
   */
  std::pair<int, Vertex> findPathMin(Vertex u);

  /**
     Add an integer value to all vertices on the path from 'u' to the root.
   */
  void updatePath(Vertex u, int delta);
};
}; // namespace LinkCut
