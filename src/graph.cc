//
// Copyright [2020] <inhzus>
//

#include "regex/graph.h"

#include <cassert>
#include <queue>
#include <stack>

#include "regex/regex.h"

namespace regex {

struct Char {
  static const char kMore = '*', kConcat = '.', kQuest = '?', kPlus = '+';
};

bool Node::IsMatch() const {
  if (Match == status) return true;
  std::stack<const Node *, std::vector<const Node *>> stack;
  auto push_successor = [&stack = stack](const Node *node) {
    for (const Edge &edge : node->edges) {
      if (edge.IsEmpty()) {
        stack.push(edge.next);
      }
    }
  };
  push_successor(this);
  while (!stack.empty()) {
    const Node *node = stack.top();
    stack.pop();
    if (Match == node->status) return true;
    push_successor(node);
  }
  return false;
}

Graph Graph::Compile(const std::string &s) {
  std::stack<Segment> stack;
  std::vector<Node *> nodes;
  for (char ch : s) {
    switch (ch) {
      case Char::kMore: {
        Segment elem(stack.top());
        stack.pop();
        auto start = new Node({Edge(Edge::Empty, elem.start)});
        nodes.push_back(start);
        Segment seg(start, start);
        std::for_each(
            elem.ends.begin(), elem.ends.end(),
            [&seg = seg](Node *node) {
              node->edges.emplace_back(Edge::Empty, seg.start);
            });
        stack.push(std::move(seg));
        break;
      }
      case Char::kConcat: {
        Segment back(stack.top());
        stack.pop();
        Segment &front(stack.top());
        std::for_each(
            front.ends.begin(), front.ends.end(),
            [&back = back](Node *node) {
              node->edges.emplace_back(Edge::Empty, back.start);
            });
        front.ends = std::move(back.ends);
        break;
      }
      case Char::kQuest: {
        break;
      }
      case Char::kPlus: {
        break;
      }
      default: {
        auto node = new Node();
        nodes.push_back(node);
        auto next = new Node;
        nodes.push_back(next);
        node->edges.emplace_back(ch, next);
        Segment segment(node, next);
        stack.push(std::move(segment));
        break;
      }
    }
  }
  assert(stack.size() == 1);
  auto match = new Node;
  match->status = Node::Match;
  nodes.push_back(match);
  Segment &seg(stack.top());
  for (auto &node : seg.ends) {
    node->edges.emplace_back(Edge::Empty, match);
  }
//  match->ref += seg.ends.size();
  seg.ends.clear();
  return Graph(std::move(seg), std::move(nodes));
}

Graph::~Graph() {
  for (Node *node : nodes_) {
    delete node;
  }
}

int Graph::Match(const std::string &s) const {
  std::stack<std::pair<
      std::string::const_iterator, Node *>> stack;
  stack.push(std::make_pair(s.begin(), seg_.start));
  while (!stack.empty()) {
    auto[it, node] = stack.top();
    stack.pop();
//    bool is_match = node->IsMatch();
//    bool is_match = node->status == Node::Match;
    if (s.end() == it) {
      if (node->IsMatch())
        return it - s.begin();
      else
        continue;
    }
//    for (Edge &edge : node->edges) {
//      std::string::const_iterator tit = it;
//      if (!EdgeMatchStrIt(edge, &tit)) continue;  // if fail, go next
//      stack.push(std::make_pair(tit, edge.next));
//    }
    for (auto edge = node->edges.rbegin(); edge != node->edges.rend(); ++edge) {
      std::string::const_iterator tit = it;
      if (!EdgeMatchStrIt(*edge, &tit)) continue;
      stack.push(std::make_pair(tit, edge->next));
    }
//    std::for_each(
//        node->edges.rbegin(), node->edges.rend(),
//        [&stack = stack, &it = it](const Edge &edge) {
//          if (std::string::const_iterator tit = it;
//              EdgeMatchStrIt(edge, &tit)) {
//            stack.push(std::make_pair(tit, edge.next));
//          }
//        });
//    if (is_match) return it - s.begin();
    if (Node::Match == node->status) return it - s.begin();
  }
  return -1;
}

#define FallThrough do {} while (0)
bool Graph::EdgeMatchStrIt(const Edge &edge, std::string::const_iterator *it) {
  switch (edge.type) {
    case Edge::Any:++*it;
      FallThrough;
    case Edge::Empty:return true;
    case Edge::Char: {
      if (**it == edge.ch) {
        ++*it;
        return true;
      } else {
        return false;
      }
    }
    default: assert(false);
  }
}

}  // namespace regex
