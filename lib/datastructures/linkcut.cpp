#include "linkcut.hpp"

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
    u->right->pathparent = u;
    u->right->parent = nullptr;
    u->right = nullptr;
  }

  while (u->pathparent) {
    SplayTree::Vertex *v = u->pathparent;
    v->splay();

    if (v->right) {
      // Fix v's right child before replacing it with u.
      v->right->pathparent = v;
      v->right->parent = nullptr;
    }

    v->right = u;
    u->parent = v;
    u->pathparent = nullptr;

    u->splay();
  }
}

void Forest::link(Vertex from, Vertex to, int weight) {
  access(from);
  access(to);

  SplayTree::Vertex *u = &vertices[from], *v = &vertices[to];

  assert(!u->left && "'u' already has a parent in represented tree.");
  u->left = v;
  assert(!v->parent && "'v' already has a parent in represented tree.");
  v->parent = u;
}

Vertex Forest::cut(Vertex vertex) {
  access(vertex);
  SplayTree::Vertex *u = &vertices[vertex];
  SplayTree::Vertex *v = u->left;

  v->parent = nullptr;
  u->left = nullptr;

  return v->id;
}

bool Forest::connected(Vertex u, Vertex v) {
  return findRoot(u) == findRoot(v);
}

Vertex Forest::findRoot(Vertex vertex) {
  access(vertex);
  const SplayTree::Vertex *r = vertices[vertex].findPathTop();
  access(r->id);
  return r->id;
}

std::pair<int, Vertex> Forest::findPathMin(Vertex vertex) {
  access(vertex);
  const auto [minVal, v] = vertices[vertex].findPathMin();
  return {minVal, v->id};
}

} // namespace LinkCut
