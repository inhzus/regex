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
  enum Type { Char, Any, Epsilon };

  static Edge EpsilonEdge(Node *node) {
    return Edge(Epsilon, node);
  }

  Edge(Type type, Node *node) : ch(), type(type), next(node) {}
  Edge(char ch, Node *node) : ch(ch), type(Char), next(node) {}

  [[nodiscard]] bool IsEmpty() const { return Epsilon == type; }

  char ch;
  Type type;
  Node *next;
};

struct Node {
  enum Status { Default, Match };

  Node() : status(Default) {}
  explicit Node(std::vector<Edge> edges) :
      status(Default), edges(std::move(edges)) {}
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
  static Graph Compile(const std::string &s);

  Graph(const Segment &seg, std::vector<Node *> &&nodes) :
      seg_(seg), nodes_(nodes) {}
  ~Graph();

  [[nodiscard]] int Match(const std::string &s) const;

 private:
  static bool EdgeMatchStrIt(const Edge &edge, std::string::const_iterator *it);

  Segment seg_;
  std::vector<Node *> nodes_;
};

}  // namespace regex

#endif  // REGEX_GRAPH_H_
