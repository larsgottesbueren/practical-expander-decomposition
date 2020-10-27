#pragma once

#include <ostream>
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
     Return the weight of a vertex.
   */
  int get(Vertex u);

  /**
     Set a weight of a vertex.
   */
  void set(Vertex u, int value);

  /**
     Add a directed edge '(u,v)' with a certain weight. This makes 'v' the
     parent of 'u' in the rooted forest.

     Precondition: 'u' does not have a parent.
   */
  void link(Vertex u, Vertex v, int weight);

  /**
     Cut the edge '(u,p(u))' where 'p(u)' is the parent of 'u'. Returns 'p(u)'
     or -1 if no parent exists.
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
     Return parent of vertex or -1 if no such parent exists.
   */
  Vertex findParent(Vertex u);

  /**
     Find the edge in a path which points to the root.

     Example: With the following directed path 'findRootEdge(a)' should result
       in vertex 'd' since edges are represented by their tail.

       a -> b -> c -> d -> root
   */
  Vertex findRootEdge(Vertex u);

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

  /**
     Add an integer value to all ledges along a path from 'u' to the root.
   */
  void updatePathEdges(Vertex u, int delta);

  /**
     Iterate over subset of vertices and clear all parent and child pointers as
     well as setting weights to 0.
   */
  template <typename It> void reset(It begin, It end) {
    for (auto it = begin; it != end; ++it)
      vertices[*it].reset();
  }

  /**
     Print available information to the output stream without modifying the
     datastructure.
   */
  friend std::ostream &operator<<(std::ostream &os, const Forest &f) {
    int n = (int)f.vertices.size();
    os << "Link-cut forest of size " << n << ":" << std::endl;

    for (int i = 0; i < n; ++i) {
      const auto &v = f.vertices[i];
      os << i << std::endl;
      if (v.parent)
        os << "\tParent: " << v.parent->id << std::endl;
      if (v.pathparent)
        os << "\tPath parent: " << v.pathparent->id << std::endl;
      if (v.left)
        os << "\tLeft: " << v.left->id << std::endl;
      if (v.right)
        os << "\tRight: " << v.right->id << std::endl;
    }
    return os;
  }
};
}; // namespace LinkCut
