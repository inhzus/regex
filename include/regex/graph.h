//
// Copyright [2020] <inhzus>
//
#ifndef REGEX_GRAPH_H_
#define REGEX_GRAPH_H_

#include <cstring>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "regex/exp.h"

namespace regex {

struct Node;
class Graph;

struct Edge {
  enum Type {
    Empty, Ahead, NegAhead, Any, Brake, Char, Epsilon, Lower, Store, StoreEnd,
    Named, NamedEnd, Repeat, Upper
  };

  static Edge AheadEdge(Node *next, Graph *graph) {
    return Edge(Ahead, next, graph);
  }
  static Edge NegAheadEdge(Node *next, Graph *graph) {
    return Edge(NegAhead, next, graph);
  }
  static Edge CharEdge(Node *next, char ch) {
    return Edge(Char, next, ch);
  }
  static Edge AnyEdge(Node *next) {
    return Edge(Any, next);
  }
  static Edge EpsilonEdge(Node *next) {
    return Edge(Epsilon, next);
  }
  static Edge BrakeEdge(
      Node *next, bool *pass, std::function<void()> *initializer) {
    return Edge(Brake, next, pass, initializer);
  }
  static Edge LowerEdge(Node *next, size_t *repeat, size_t num) {
    return Edge(Lower, next, repeat, num);
  }
  static Edge StoreEdge(Node *next, size_t idx) {
    return Edge(Store, next, idx);
  }
  static Edge StoreEndEdge(Node *next, size_t idx) {
    return Edge(StoreEnd, next, idx);
  }
  static Edge NamedEdge(Node *next, size_t idx, const std::string &s) {
    return Edge(Named, next, idx, s);
  }
  static Edge NamedEndEdge(Node *next, size_t idx, const std::string &s) {
    return Edge(NamedEnd, next, idx, s);
  }
  static Edge RepeatEdge(Node *next, size_t *repeat,
                         std::function<void()> *initializer) {
    return Edge(Repeat, next, repeat, initializer);
  }
  static Edge UpperEdge(Node *next, size_t *repeat, size_t num) {
    return Edge(Upper, next, repeat, num);
  }
  Edge(const Edge &) = delete;
  Edge &operator=(const Edge &) = delete;
  Edge(Edge &&) noexcept;
  Edge &operator=(Edge &&) = delete;
  ~Edge();

  [[nodiscard]] bool IsEpsilon() const { return Epsilon == type; }

  Type type;
  Node *next;
  union {
    struct {
      char val;
    } ch;
    struct {
      Graph *graph;
    } ahead, neg_ahead;
    struct {
      bool *pass;
    } brake;
    struct {
      size_t idx;
    } store, store_end;
    struct {
      size_t *val;
    } repeat;
    struct {
      size_t idx;
      char *name;
    } named, named_end;
    struct {
      size_t *repeat;
      size_t num;
    } bound;
  };

 private:
  Edge(Type type, Node *next) : type(type), next(next), ch({0}) {}
  Edge(Type type, Node *next, char ch) : type(type), next(next), ch({ch}) {}
  Edge(Type type, Node *next, Graph *graph) :
      type(type), next(next), ahead({graph}) {}
  Edge(Type type, Node *next, size_t idx) :
      type(type), next(next), store({idx}) {}
  Edge(Type type, Node *next, size_t idx, const std::string &s) :
      type(type), next(next), named({idx, nullptr}) {
    named.name = new char[s.size() + 1];
    snprintf(named.name, s.size() + 1, "%s", s.c_str());
  }
  Edge(Type type, Node *next, bool *pass, std::function<void()> *initializer) :
      type(type), next(next), brake({pass}) {
    *initializer = [pass = pass]() {
      *pass = true;
    };
  }
  Edge(Type type, Node *next, size_t *repeat, size_t num) :
      type(type), next(next), bound({repeat, num}) {}
  Edge(Type type, Node *next, size_t *repeat,
       std::function<void()> *initializer) :
      type(type), next(next), repeat({repeat}) {
    *initializer = [repeat = repeat]() {
      *repeat = 0;
    };
  }
};

struct Node {
  enum Status { Default, Match };

  Node() : status(Default) {}
//  explicit Node(const std::vector<Edge> &edges) :
//      status(Default), edges(edges) {}
  explicit Node(Edge &&edge) : status(Default) {
    edges.push_back(std::move(edge));
  }
  explicit Node(Edge &&e1, Edge &&e2) : status(Default) {
    edges.push_back(std::move(e1));
    edges.push_back(std::move(e2));
  }
//  explicit Node(std::vector<Edge> &&edges) :
//      status(Default), edges(std::move(edges)) {}
//  Node(Status status, std::vector<Edge> edges) :
//      status(status), edges(std::move(edges)) {}

//  static Node MatchNode() {
//    return Node(Match, std::vector<Edge>());
//  }

  [[nodiscard]] bool WillMatch() const;

  Status status;
  std::vector<Edge> edges;
};

struct Segment {
  Segment(Node *start, Node *end) :
      start(start), end(end) {}

  Node *start;
  Node *end;
};

class Graph {
 public:
  static Graph CompilePostfix(const std::string &s);
  static Graph Compile(const std::string &s);
  static Graph Compile(Exp &&exp);

  Graph(const Graph &) = delete;
  Graph operator=(const Graph &) = delete;
  Graph(Graph &&) = default;
  Graph &operator=(Graph &&graph) noexcept {
    Deallocate();
    group_num_ = graph.group_num_;
    seg_ = graph.seg_;
    nodes_ = std::move(graph.nodes_);
    initializers_ = std::move(graph.initializers_);
    return *this;
  }
  Graph(const Segment &seg, std::vector<Node *> &&nodes,
        std::vector<std::function<void()>> &&initializers, size_t group_num) :
      group_num_(group_num), seg_(seg), nodes_(nodes),
      initializers_(initializers) {}
  ~Graph() { Deallocate(); }

  [[nodiscard]] int Match(const std::string &s) const;
  [[nodiscard]] int
  Match(const std::string &s, std::vector<std::string> *groups) const;
  void DrawMermaid() const;

 private:
  void Deallocate();

  size_t group_num_;
  Segment seg_;
  std::vector<Node *> nodes_;
  std::vector<std::function<void()>> initializers_;
};

}  // namespace regex

#endif  // REGEX_GRAPH_H_
