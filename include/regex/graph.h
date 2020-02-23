//
// Copyright [2020] <inhzus>
//
#ifndef REGEX_GRAPH_H_
#define REGEX_GRAPH_H_

#include <cstring>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "regex/id.h"

namespace regex {

struct Node;

struct Edge {
  enum Type { Any, Char, Epsilon, Store, StoreEnd, Named, NamedEnd };

  static Edge CharEdge(char ch, Node *node) {
    return Edge(Char, node, ch);
  }
  static Edge AnyEdge(Node *next) {
    return Edge(Any, next);
  }
  static Edge EpsilonEdge(Node *next) {
    return Edge(Epsilon, next);
  }
  static Edge StoreEdge(int idx, Node *next) {
    return Edge(Store, next, idx);
  }
  static Edge StoreEndEdge(int idx, Node *next) {
    return Edge(StoreEnd, next, idx);
  }
  static Edge NamedEdge(int idx, const std::string &s, Node *next) {
    return Edge(Named, next, idx, s);
  }
  static Edge NamedEndEdge(int idx, const std::string &s, Node *next) {
    return Edge(NamedEnd, next, idx, s);
  }
  ~Edge() {
    if (type == Named || type == NamedEnd) {
      delete named.name;
    }
  }

  [[nodiscard]] bool IsEpsilon() const { return Epsilon == type; }

  Type type;
  Node *next;
  union {
    struct {
      char val;
    } ch;
    struct {
      int idx;
    } store, store_end;
    struct {
      int idx;
      char *name;
    } named, named_end;
  };

 private:
  Edge(Type type, Node *next) : type(type), next(next), ch({0}) {}
  Edge(Type type, Node *next, char ch) : type(type), next(next), ch({ch}) {}
  Edge(Type type, Node *next, int idx) : type(type), next(next), store({idx}) {}
  Edge(Type type, Node *next, int idx, const std::string &s) :
      type(type), next(next), named({idx, nullptr}) {
    named.name = new char[s.size() + 1];
    snprintf(named.name, s.size() + 1, "%s", s.c_str());
  }
};

struct Node {
  enum Status { Default, Match };

  Node() : status(Default) {}
//  explicit Node(const std::vector<Edge> &edges) :
//      status(Default), edges(edges) {}
  explicit Node(std::vector<Edge> &&edges) :
      status(Default), edges(edges) {}
//  Node(Status status, std::vector<Edge> edges) :
//      status(status), edges(std::move(edges)) {}

//  static Node MatchNode() {
//    return Node(Match, std::vector<Edge>());
//  }

  [[nodiscard]] bool IsMatch() const;

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
  static Graph Compile(std::vector<Id> &&ids);

  Graph(const Graph &) = delete;
  Graph operator=(const Graph &) = delete;
  Graph(Graph &&) = default;
  Graph &operator=(Graph &&graph) noexcept {
    Deallocate();
    seg_ = graph.seg_;
    nodes_ = std::move(graph.nodes_);
    return *this;
  }
  Graph(const Segment &seg, std::vector<Node *> &&nodes) :
      seg_(seg), nodes_(nodes) {}
  ~Graph() { Deallocate(); }

  void swap(Graph &graph) {
    using std::swap;
    swap(seg_, graph.seg_);
    swap(nodes_, graph.nodes_);
  }

  [[nodiscard]] int Match(const std::string &s) const;
  [[nodiscard]] int
  Match(const std::string &s, std::vector<std::string> *groups) const;
  void DrawMermaid() const;

 private:
  static bool EdgeMatchStrIt(const Edge &edge, std::string::const_iterator *it);
  void Deallocate();

  Segment seg_;
  std::vector<Node *> nodes_;
};

}  // namespace regex

#endif  // REGEX_GRAPH_H_
