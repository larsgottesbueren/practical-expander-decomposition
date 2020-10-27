#include "linkcut.hpp"

#include <algorithm>
#include <iostream>
#include <limits>

namespace LinkCut {

Forest::Forest(int n) : vertices(n, SplayTree::Vertex(-1)) {
  for (int i = 0; i < n; ++i)
    vertices[i].id = i;
}

void Forest::access(Vertex vertex) {
  SplayTree::Vertex *u = &vertices[vertex];
  u->splay();

  if (u->right) {
    // Fix right child.
    u->right->deltaW += u->deltaW;
    u->right->pathparent = u;
    u->right->parent = nullptr;
    u->right = nullptr;
  }

  while (u->pathparent) {
    SplayTree::Vertex *v = u->pathparent;
    v->splay();

    if (v->right) {
      // Fix v's right child before replacing it with u.
      v->right->deltaW += v->deltaW;
      v->right->pathparent = v;
      v->right->parent = nullptr;
    }

    v->right = u;
    u->parent = v;
    u->pathparent = nullptr;
    u->deltaW -= v->deltaW;
    v->updateDeltaMin();

    u->splay();
  }
}

int Forest::get(Vertex u) {
  access(u);
  return vertices[u].deltaW;
}

void Forest::set(Vertex vertex, int value) {
  access(vertex);
  SplayTree::Vertex *u = &vertices[vertex];

  if (u->left)
    u->left->deltaW += u->deltaW - value;
  if (u->right)
    u->right->deltaW += u->deltaW - value;
  u->deltaW = value;
  u->updateDeltaMin();
}

void Forest::link(Vertex from, Vertex to, int weight) {
#ifdef DEBUG
  if (connected(from, to)) {
    std::cout << *this;
    assert(false && "Attempting to link already connected vertices");
  }
#endif
  access(from);
  access(to);

  SplayTree::Vertex *u = &vertices[from], *v = &vertices[to];

  assert(!u->left && "'u' already has a parent in represented tree.");
  u->left = v;
  assert(!v->parent && "'v' should not have parent after 'access'.");
  v->parent = u;

  v->deltaW -= u->deltaW;
  u->updateDeltaMin();

  updatePath(from, weight);
  updatePath(to, -weight);
}

Vertex Forest::cut(Vertex vertex) {
  access(vertex);
  SplayTree::Vertex *u = &vertices[vertex];
  SplayTree::Vertex *v = u->left;

  if (!v)
    return -1;

  v->parent = nullptr;
  u->left = nullptr;

  v->deltaW += u->deltaW;
  u->updateDeltaMin();
  v->updateDeltaMin();

  return v->id;
}

bool Forest::connected(Vertex u, Vertex v) {
  return findRoot(u) == findRoot(v);
}

Vertex Forest::findRoot(Vertex vertex) {
  access(vertex);
  SplayTree::Vertex *u = &vertices[vertex];
  while (u->left)
    u = u->left;
  access(u->id);
  return u->id;
}

Vertex Forest::findRootEdge(Vertex vertex) {
  access(vertex);

  SplayTree::Vertex *u = &vertices[vertex];
  assert(u->left && "'findRootEdge' is undefined for root vertex");

  while (u->left && u->left->left)
    u = u->left;
  if (u->left->right) {
    u = u->left->right;
    while (u->left)
      u = u->left;
  }

  access(u->id);
  return u->id;
}

Vertex Forest::findParent(Vertex vertex) {
  access(vertex);
  SplayTree::Vertex *u = &vertices[vertex];
  return u->left ? u->left->id : -1;
}

std::pair<int, Vertex> Forest::findPathMin(Vertex vertex) {
  access(vertex);

  SplayTree::Vertex *u = &vertices[vertex];
  int weight = u->deltaW;

  if (u->left == nullptr)
    return {weight, u->id};

  // minWeight := minimum of root of aux-tree and left branch of aux-tree. Don't
  // consider right side of aux-tree as that is further down path in represented
  // tree.
  const int minWeight =
      std::min(weight, weight + u->left->deltaW + u->left->deltaMin);
  const SplayTree::Vertex *minVertex = weight == minWeight ? u : nullptr;
  u = u->left;
  weight += u->deltaW;

  if (weight == minWeight)
    minVertex = u;

  while (u->left || u->right) {
    if (u->left) {
      const int lWeight = weight + u->left->deltaW;
      const int lMin = lWeight + u->left->deltaMin;
      if (lWeight == minWeight)
        minVertex = u->left;
      if (lMin == minWeight) {
        u = u->left;
        weight = lWeight;
        continue;
      }
    }
    if (u->right && u != minVertex) {
      const int rWeight = weight + u->right->deltaW;
      const int rMin = rWeight + u->right->deltaMin;
      if (rWeight == minWeight)
        minVertex = u->right;
      if (rMin == minWeight) {
        u = u->right;
        weight = rWeight;
        continue;
      }
    }
    break;
  }
  return {minWeight, minVertex->id};
}

void Forest::updatePath(Vertex vertex, int delta) {
  access(vertex);
  SplayTree::Vertex *u = &vertices[vertex];
  u->deltaW += delta;
  if (u->right)
    u->right->deltaW -= delta;
  u->updateDeltaMin();
}

void Forest::updatePathEdges(Vertex vertex, int delta) {
  access(vertex);
  SplayTree::Vertex *u = &vertices[vertex];
  u->deltaW += delta;
  while (u->left)
    u = u->left;
  u->deltaW -= delta;
  while (u->parent)
    u->parent->updateDeltaMin(), u = u->parent;
}
} // namespace LinkCut
