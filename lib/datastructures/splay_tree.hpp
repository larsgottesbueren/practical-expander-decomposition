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

     Invariant: 'u.minWeight = u.weight + u.deltaMin'
   */
  int deltaMin;

  Vertex(int id, Vertex *left, Vertex *right, Vertex *parent,
         Vertex *pathparent, int deltaW, int deltaMin)
      : id(id), left(left), right(right), parent(parent),
        pathparent(pathparent), deltaW(deltaW), deltaMin(deltaMin) {}
  Vertex(int id) : Vertex(id, nullptr, nullptr, nullptr, nullptr, 0, 0) {}

  /**
     Rotate current vertex upwards. Let 'u' be the current vertex, 'p' the
     parent, and A,B,C be subtrees. Then a rotation of 'u' is one of the
     following:

         p         u       |     p           u
        / \       / \      |    / \         / \
       u   C  => A   p     |   A  u   =>   p  C
      / \           / \    |     / \      / \
     A   B         B   C   |    B   C    A  B

     Maintains 'deltaW' and 'deltaMin'.
   */
  void rotateUp();

  /**
     Splay this vertex.
   */
  void splay();

  /**
     Update the 'deltaMin' property of the current vertex assuming its children
     maintain the 'deltaMin' invariant.
   */
  void updateDeltaMin();
};
} // namespace SplayTree
