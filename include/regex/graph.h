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
#include <unordered_map>

#include "regex/exp.h"

namespace regex {

struct Node;
class Graph;

struct Edge {
  enum Type {
    Empty, Ahead, NegAhead, Any, Brake, Char, Epsilon, Func, Lower, Store,
    StoreEnd, Named, NamedEnd, Ref, Repeat, Set, SetEx, Upper
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
  static Edge BrakeEdge(Node *next, bool *pass) {
    return Edge(Brake, next, pass);
  }
  static Edge FuncEdge(Node *next, std::function<void()> *f) {
    return Edge(Func, next, f);
  }
  static Edge LowerEdge(Node *next, size_t *repeat, size_t num) {
    return Edge(Lower, next, repeat, num);
  }
  static Edge RefEdge(Node *next, size_t idx) {
    return Edge(Ref, next, idx);
  }
  static Edge StoreEdge(Node *next, size_t idx) {
    return Edge(Store, next, idx);
  }
  static Edge StoreEndEdge(Node *next, size_t idx) {
    return Edge(StoreEnd, next, idx);
  }
  static Edge NamedEdge(Node *next, size_t idx) {
    return Edge(Named, next, idx);
  }
  static Edge NamedEndEdge(Node *next, size_t idx) {
    return Edge(NamedEnd, next, idx);
  }
  static Edge RepeatEdge(Node *next, size_t *repeat) {
    return Edge(Repeat, next, repeat);
  }
  static Edge SetEdge(Node *next, CharSet &&v) {
    return Edge(Set, next, std::move(v));
  }
  static Edge SetExEdge(Node *next, CharSet &&v) {
    return Edge(SetEx, next, std::move(v));
  }
  static Edge UpperEdge(Node *next, size_t *repeat, size_t num) {
    return Edge(Upper, next, repeat, num);
  }
  Edge(const Edge &) = delete;
  Edge &operator=(const Edge &) = delete;
  Edge(Edge &&) noexcept;
  Edge &operator=(Edge &&) noexcept;
  ~Edge();

  [[nodiscard]] bool IsEpsilon() const { return Epsilon == type; }

  Type type;
  Node *next;
  union {
    struct {
      size_t *repeat;
      size_t num;
    } *bound;
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
      std::function<void()> *f;
    } func;
    struct {
      size_t idx;
    } ref, store, store_end, named, named_end;
    struct {
      size_t *val;
    } repeat;
    struct {
      CharSet val;
    } *set;
  };

 private:
  Edge(Type type, Node *next) : type(type), next(next), ch({0}) {}
  Edge(Type type, Node *next, char ch) : type(type), next(next), ch({ch}) {}
  Edge(Type type, Node *next, Graph *graph) :
      type(type), next(next), ahead({graph}) {}
  Edge(Type type, Node *next, size_t idx) :
      type(type), next(next), store({idx}) {}
  Edge(Type type, Node *next, std::function<void()> *f) :
      type(type), next(next), func({f}) {}
  Edge(Type type, Node *next, bool *pass) :
      type(type), next(next), brake({pass}) {}
  Edge(Type type, Node *next, size_t *repeat, size_t num) :
      type(type), next(next) {
    bound = new std::remove_reference_t<decltype(*bound)>({repeat, num});
  }
  Edge(Type type, Node *next, size_t *repeat) :
      type(type), next(next), repeat({repeat}) {}
  Edge(Type type, Node *next, CharSet &&v) :
      type(type), next(next) {
    set = new std::remove_reference_t<decltype(*set)>();
    set->val = std::move(v);
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

class Matcher {
 public:
  friend class Graph;

  Matcher(size_t group_num,
          std::unordered_map<std::string, size_t> named_group) :
      ok_(false), groups_(group_num, std::string()),
      named_groups_(std::move(named_group)) {}
  explicit operator bool() const { return ok_; }

  [[nodiscard]] std::string_view
  Group(size_t idx) const { return groups_[idx]; }
  [[nodiscard]] std::string_view
  Group(const std::string &key) const {
    auto pair_it = named_groups_.find(key);
    if (pair_it == named_groups_.end()) return kEnd;
    return groups_[pair_it->second];
  }

  [[nodiscard]] bool ok() const { return ok_; }
  [[nodiscard]] const std::vector<std::string> &
  groups() const { return groups_; }

  static const char *kEnd;

 private:
  bool ok_;
  std::vector<std::string> groups_;
  std::unordered_map<std::string, size_t> named_groups_;
};

class Graph {
 public:
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
    named_group_ = std::move(graph.named_group_);
    return *this;
  }
  Graph(size_t group_num, const Segment &seg, std::vector<Node *> &&nodes,
        std::unordered_map<std::string, size_t> named_group) :
      group_num_(group_num), seg_(seg), nodes_(std::move(nodes)),
      named_group_(std::move(named_group)) {}
  ~Graph() { Deallocate(); }

  [[nodiscard]] int MatchLen(const std::string &s) const;
  [[nodiscard]] bool
  MatchGroups(const std::string &s, std::vector<std::string> *groups) const;
  void Match(const std::string &s, Matcher *matcher) const;
  Matcher Match(const std::string &s) const;
  void DrawMermaid() const;

 private:
  void Deallocate();

  size_t group_num_;
  Segment seg_;
  std::vector<Node *> nodes_;
  std::unordered_map<std::string, size_t> named_group_;
};

}  // namespace regex

#endif  // REGEX_GRAPH_H_
