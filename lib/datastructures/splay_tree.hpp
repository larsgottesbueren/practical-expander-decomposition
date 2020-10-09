#pragma once

#include <utility>

namespace SplayTree {
class Vertex {
public:
  /** Unique identifier. */
  int id;
  /** Left and right nodes in the represented tree. */
  Vertex *left, *right;
  /** Parent in the auxillary tree. */
  Vertex *parent;
  /**
Path parent. This is the parent in the represented tree of the left-most
      vertex in the current aux-tree.
 */
  Vertex *pathparent;
  /**
     Difference in between this vertex and parent. For the root vertex we have
     the invariant 'root.w = root.deltaW'. Maintaining deltas instead of actual
     values makes updating entire subtrees fast.
   */
  int deltaW;
  /**
     Difference between subtrees minimum value and weight at current node.

     Definition: 'u.minDiff = u.minWeight - u.weight'
   */
  int minDiff;

  Vertex(int id, Vertex *left, Vertex *right, Vertex *parent,
         Vertex *pathparent, int deltaW, int minDiff)
      : id(id), left(left), right(right), parent(parent),
        pathparent(pathparent), deltaW(deltaW), minDiff(minDiff) {}
  Vertex(int id) : Vertex(id, nullptr, nullptr, nullptr, nullptr, 0, 0) {}

  /**
     Rotate current vertex upwards. Let 'u' be the current vertex, 'p' the
     parent, and A,B,C be subtrees. Then a rotation of 'u' is:

         p         u
        / \       / \
       u   C  => A   p
      / \           / \
     A   B         B   C
   */
  void rotateUp();

  /**
     Splay this vertex.
   */
  void splay();

  /**
     Find top of path (closest to root) represented by this aux-tree.
   */
  Vertex *findPathTop();

  /**
     Find minimum value on path. Return value along with vertex closest to root
     which has this value.
   */
  std::pair<int, Vertex *> findPathMin();

  /**
     Increment all values on path with delta.
   */
  void updatePath(int delta);
};
} // namespace SplayTree
