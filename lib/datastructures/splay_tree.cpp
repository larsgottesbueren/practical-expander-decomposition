#include "splay_tree.hpp"

#include <cassert>

namespace SplayTree {

void Vertex::rotateUp() {
  Vertex *u = this, *p = parent;
  if (!p)
    return;

  u->pathparent = p->pathparent;
  p->pathparent = nullptr;

  if (p->left == u) {
    p->left = u->right;
    if (p->left)
      p->left->parent = p;

    u->right = p, u->parent = p->parent;
    p->parent = u;
  } else {
    p->right = u->left;
    if (p->right)
      p->right->parent = p;

    u->left = p, u->parent = p->parent;
    p->parent = u;
  }

  if (u->parent) {
    if (u->parent->left == p)
      u->parent->left = u;
    else if (u->parent->right == p)
      u->parent->right = u;
  }
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

Vertex *Vertex::findPathTop() {
  Vertex *u = this;
  while (u->left)
    u = u->left;
  return u;
}

std::pair<int, Vertex *> Vertex::findPathMin() { assert(false); }

void Vertex::updatePath(int delta) { assert(false); }

} // namespace SplayTree
