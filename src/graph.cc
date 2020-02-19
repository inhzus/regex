//
// Copyright [2020] <inhzus>
//

#include "regex/graph.h"

#include <cassert>
#include <queue>
#include <stack>
#include <unordered_map>

#include "regex/regex.h"

namespace regex {

struct Char {
  static const char kConcat = '.', kEither = '|', kMore = '*',
      kPlus = '+', kQuest = '?';
};

bool Node::IsMatch() const {
  if (Match == status) return true;
  std::stack<const Node *, std::vector<const Node *>> stack;
  auto push_successor = [&stack = stack](const Node *node) {
    for (const Edge &edge : node->edges) {
      if (edge.IsEpsilon()) {
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

#define FallThrough do {} while (0)
Graph Graph::Compile(const std::string &s) {
  std::stack<Segment> stack;
  std::vector<Node *> nodes;
  for (auto sit = s.begin(); sit != s.end(); ++sit) {
    char ch = *sit;
//  for (char ch : s) {
    bool greedy = true;
    switch (ch) {
      case Char::kMore:
      case Char::kPlus:
      case Char::kQuest: {
        // lazy mode: *?, +?, ??
        auto next(sit + 1);
        if (next != s.end() && *next == Char::kQuest) {
          ++sit;
          greedy = false;
        }
        break;
      }
      default: break;
    }
    switch (ch) {
      case Char::kConcat: {
        //      | left | right |
        // start=0==>0-->0==>end=0
        Segment back(stack.top());
        stack.pop();
        Segment &front(stack.top());
        front.end->edges.push_back(Edge::EpsilonEdge(back.start));
        front.end = back.end;
        break;
      }
      case Char::kEither: {
        //       |   left    |
        //       |-->0==>0-->|
        // start=0       end=0
        //       |-->0==>0-->|
        //       |   right   |
        Segment right(stack.top());
        stack.pop();
        Segment left(stack.top());
        stack.pop();
        auto start = new Node(
            {Edge::EpsilonEdge(left.start),
             Edge::EpsilonEdge(right.start)});
        nodes.push_back(start);
        auto end = new Node;
        nodes.push_back(end);
        left.end->edges.push_back(Edge::EpsilonEdge(end));
        right.end->edges.push_back(Edge::EpsilonEdge(end));
        stack.push(Segment(start, end));
        break;
      }
      case Char::kMore: {
        //       |-->0==>0-->|
        // start=0<--.<--.<--|   |-->end=0
        //       |-->.-->.-->.-->|
        Segment elem(stack.top());
        stack.pop();
        auto end = new Node;
        nodes.push_back(end);
        std::vector<Edge> edges;
        if (greedy) {
          edges = {Edge::EpsilonEdge(elem.start),
                   Edge::EpsilonEdge(end)};
        } else {
          edges = {Edge::EpsilonEdge(end),
                   Edge::EpsilonEdge(elem.start)};
        }
        auto start = new Node(std::move(edges));
        nodes.push_back(start);
        elem.end->edges.push_back(Edge::EpsilonEdge(start));
        stack.push(Segment(start, end));
        break;
      }
      case Char::kPlus: {
        break;
      }
      case Char::kQuest: {
        //       |   elem    |
        //       |-->0==>0-->|
        // start=0       end=0
        //       |-->.-->.-->|
        Segment elem(stack.top());
        stack.pop();
        auto end = new Node;
        nodes.push_back(end);
        std::vector<Edge> edges;
        if (greedy) {
          edges = {Edge::EpsilonEdge(elem.start),
                   Edge::EpsilonEdge(end)};
        } else {
          edges = {Edge::EpsilonEdge(end),
                   Edge::EpsilonEdge(elem.start)};
        }
        auto start = new Node(std::move(edges));
        nodes.push_back(start);
        elem.end->edges.push_back(Edge::EpsilonEdge(end));
        stack.push(Segment(start, end));
        break;
      }
      default: {
        // start=0-->ch=0-->end=0
        auto start = new Node;
        nodes.push_back(start);
        auto end = new Node;
        nodes.push_back(end);
        start->edges.emplace_back(ch, end);
        stack.push(Segment(start, end));
        break;
      }
    }
  }
  assert(stack.size() == 1);
  Segment &seg(stack.top());
  seg.end->status = Node::Match;
  return Graph(seg, std::move(nodes));
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

void Graph::DrawMermaid() const {
  int id = 0;
  std::unordered_map<Node *, int> map{{seg_.start, id++}};
  std::stack<Node *> stack;
  stack.push(seg_.start);
  while (!stack.empty()) {
    Node *node = stack.top();
    stack.pop();
    for (const Edge &edge : node->edges) {
      if (!map.contains(edge.next)) {
        map[edge.next] = id++;
        stack.push(edge.next);
      }
      if (!edge.IsEpsilon()) {
        printf("%d-->|%c|%d\n", map[node], edge.ch, map[edge.next]);
      } else if (Node::Match == edge.next->status) {
        printf("%d-->*%d\n", map[node], map[edge.next]);
      } else {
        printf("%d-->%d\n", map[node], map[edge.next]);
      }
    }
  }
}

bool Graph::EdgeMatchStrIt(const Edge &edge, std::string::const_iterator *it) {
  switch (edge.type) {
    case Edge::Any:++*it;
      FallThrough;
    case Edge::Epsilon:return true;
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
