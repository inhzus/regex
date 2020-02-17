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
  static const char kAny = '*', kConcat = '.', kQuest = '?', kPlus = '+';
};

Graph Graph::Compile(const std::string &s) {
  std::stack<Segment> stack;
  for (char ch : s) {
    switch (ch) {
      case Char::kAny: {
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
              ++back.start->ref;
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
        node->edges.emplace_back(ch, new Node(1));
        Segment segment(node, node->edges.back().next);
        stack.push(std::move(segment));
        break;
      }
    }
  }
  assert(stack.size() == 1);
  Segment &seg(stack.top());
  assert(seg.ends.size() == 1);
  seg.ends.front()->status = Node::Match;
  seg.ends.clear();
  return Graph(seg);
}

Graph::~Graph() {
  std::queue<Node *> queue;
  queue.push(seg_.start);
  while (!queue.empty()) {
    Node *node = queue.front();
    queue.pop();
    for (auto &edge : node->edges) {
      edge.next->ref--;
      queue.push(edge.next);
    }
    if (node->ref == 0)
      delete (node);
  }
}

}  // namespace regex
