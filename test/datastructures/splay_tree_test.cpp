#include "gtest/gtest.h"

#include "lib/datastructures/splay_tree.hpp"

namespace SplayTree {

TEST(SplayTree, RotateSingle) {
  Vertex *u = new Vertex(0);
  u->rotateUp();

  EXPECT_EQ(u->parent, nullptr);
  EXPECT_EQ(u->left, nullptr);
  EXPECT_EQ(u->right, nullptr);
}

TEST(SplayTree, RotateUpTwoLeft) {
  Vertex *u = new Vertex(0), *p = new Vertex(1);

  u->parent = p, p->left = u;
  u->rotateUp();

  EXPECT_EQ(u->parent, nullptr);
  EXPECT_EQ(u->left, nullptr);
  EXPECT_EQ(u->right, p);

  EXPECT_EQ(p->parent, u);
  EXPECT_EQ(p->left, nullptr);
  EXPECT_EQ(p->right, nullptr);
}

TEST(SplayTree, RotateUpTwoRight) {
  Vertex *u = new Vertex(0), *p = new Vertex(1);

  u->parent = p, p->right = u;
  u->rotateUp();

  EXPECT_EQ(u->parent, nullptr);
  EXPECT_EQ(u->left, p);
  EXPECT_EQ(u->right, nullptr);

  EXPECT_EQ(p->parent, u);
  EXPECT_EQ(p->left, nullptr);
  EXPECT_EQ(p->right, nullptr);
}

TEST(SplayTree, RotateUpFullLeft) {
  Vertex *u = new Vertex(0), *p = new Vertex(1), *a = new Vertex(2),
         *b = new Vertex(3), *c = new Vertex(4);

  u->parent = p, p->left = u;

  a->parent = u, u->left = a;
  b->parent = u, u->right = b;
  c->parent = p, p->right = c;

  u->rotateUp();

  EXPECT_EQ(u->parent, nullptr);
  EXPECT_EQ(u->left, a);
  EXPECT_EQ(u->right, p);

  EXPECT_EQ(p->parent, u);
  EXPECT_EQ(p->left, b);
  EXPECT_EQ(p->right, c);

  EXPECT_EQ(a->parent, u);
  EXPECT_EQ(b->parent, p);
  EXPECT_EQ(c->parent, p);
}

TEST(SplayTree, RotateUpFullRight) {
  Vertex *u = new Vertex(0), *p = new Vertex(1), *a = new Vertex(2),
         *b = new Vertex(3), *c = new Vertex(4);

  u->parent = p, p->right = u;

  a->parent = p, p->left = a;
  b->parent = u, u->left = b;
  c->parent = u, u->right = c;

  u->rotateUp();

  EXPECT_EQ(u->parent, nullptr);
  EXPECT_EQ(u->left, p);
  EXPECT_EQ(u->right, c);

  EXPECT_EQ(p->parent, u);
  EXPECT_EQ(p->left, a);
  EXPECT_EQ(p->right, b);

  EXPECT_EQ(a->parent, p);
  EXPECT_EQ(b->parent, p);
  EXPECT_EQ(c->parent, u);
}

}; // namespace SplayTree
