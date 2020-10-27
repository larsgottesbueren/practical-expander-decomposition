#include "splay_tree.hpp"

#include <algorithm>
#include <cassert>
#include <limits>

namespace SplayTree {

void Vertex::rotateUp() {
  Vertex *u = this, *p = parent;
  if (!p)
    return;

  u->pathparent = p->pathparent;
  p->pathparent = nullptr;

  int uDeltaW = u->deltaW, pDeltaW = p->deltaW;
  u->deltaW = uDeltaW + pDeltaW;
  p->deltaW = -uDeltaW;

  if (p->left == u) {
    p->left = u->right;
    if (p->left)
      p->left->parent = p, p->left->deltaW += uDeltaW;

    u->right = p, u->parent = p->parent;
    p->parent = u;
  } else {
    p->right = u->left;
    if (p->right)
      p->right->parent = p, p->right->deltaW += uDeltaW;

    u->left = p, u->parent = p->parent;
    p->parent = u;
  }

  if (u->parent) {
    if (u->parent->left == p)
      u->parent->left = u;
    else if (u->parent->right == p)
      u->parent->right = u;
  }

  // Notice the order, 'p' is below 'u' in tree and must be updated first.
  p->updateDeltaMin();
  u->updateDeltaMin();
}

void Vertex::splay() {
  while (parent) {
    if (parent->parent == nullptr) {
      rotateUp();
    } else {
      bool zigzigCase =
          (parent->left == this && parent->parent->left == parent) ||
          (parent->right == this && parent->parent->right == parent);
      if (zigzigCase) {
        parent->rotateUp();
        rotateUp();
      } else {
        rotateUp();
        rotateUp();
      }
    }
  }
}

void Vertex::updateDeltaMin() {
  int l = std::numeric_limits<int>::max(), r = std::numeric_limits<int>::max();
  if (left)
    l = left->deltaW + left->deltaMin;
  if (right)
    r = right->deltaW + right->deltaMin;

  deltaMin = std::min({0, l, r});
}

void Vertex::reset() {
  left = nullptr, right = nullptr, parent = nullptr, pathparent = nullptr;
  deltaW = 0, deltaMin = 0;
}

} // namespace SplayTree
