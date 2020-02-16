//
// Copyright [2020] <inhzus>
//
#ifndef REGEX_GRAPH_H_
#define REGEX_GRAPH_H_

#include <string>
#include <utility>
#include <vector>

namespace regex {

struct Edge {
  enum Type { Char, Wildcard, Empty };

  explicit Edge(Type type) : ch(), type(type) {}
  explicit Edge(char ch) : ch(ch), type(Char) {}

  char ch;
  Type type;
};

struct Node {
  enum Status { Default, Match };

  Node() : status(Default) {}
  Node(Status status, std::vector<Node *> edges) :
      status(status), edges(std::move(edges)) {}

  static Node MatchNode() {
    return Node(Match, std::vector<Node *>());
  }

  Status status;
  std::vector<Node *> edges;
};

struct Segment {
  explicit Segment(Node *start) : start(start) {}

  static Segment Parse(const std::string &s);

  Node *start;
  std::vector<Node *> ends;
};

}  // namespace regex

#endif  // REGEX_GRAPH_H_
