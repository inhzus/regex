//
// Copyright [2020] <inhzus>
//
#ifndef REGEX_GRAPH_H_
#define REGEX_GRAPH_H_

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace regex {

struct Node;

struct Edge {
  enum Type { Char, Any, Empty };

  Edge(Type type, Node *node) : ch(), type(type), next(node) {}
  Edge(char ch, Node *node) : ch(ch), type(Char), next(node) {}

  char ch;
  Type type;
  Node *next;
};

struct Node {
  enum Status { Default, Match };

  Node() : ref(0), status(Default) {}
  explicit Node(size_t ref) : ref(ref), status(Default) {}
  explicit Node(std::vector<Edge> edges) :
      ref(0), status(Default), edges(std::move(edges)) {}
  Node(size_t ref, std::vector<Edge> edges) :
      ref(ref), status(Default), edges(std::move(edges)) {}
//  Node(Status status, std::vector<Edge> edges) :
//      status(status), edges(std::move(edges)) {}

//  static Node MatchNode() {
//    return Node(Match, std::vector<Edge>());
//  }

  size_t ref;
  Status status;
  std::vector<Edge> edges;
};

struct Segment {
  explicit Segment(Node *start) : start(start) {}
  Segment(Node *start, Node *end) :
      start(start), ends({end}) {}

  Node *start;
  std::vector<Node *> ends;
};

class Graph {
 public:
  static Graph Compile(const std::string &s);

  explicit Graph(Segment seg) : seg_(std::move(seg)) {}
  ~Graph();

  bool Match(const std::string &s) const;

 private:
  Segment seg_;
};

}  // namespace regex

#endif  // REGEX_GRAPH_H_
