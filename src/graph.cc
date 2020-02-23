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
Graph Graph::CompilePostfix(const std::string &s) {
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
        start->edges.push_back(Edge::CharEdge(ch, end));
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
Graph Graph::Compile(const std::string &s) {
  return Compile(StrToPostfixIds(s));
}
Graph Graph::Compile(std::vector<Id> &&ids) {
  std::stack<Segment> stack;
  std::vector<Node *> nodes;  // for memory management
  int group_idx = 1;

  for (auto id : ids) {
    switch (static_cast<int>(id.sym)) {
      case Id::Sym::Any: {
        // stack=0-->any-->end=0
        auto end = new Node;
        nodes.push_back(end);
        auto start = new Node({Edge::AnyEdge(end)});
        nodes.push_back(start);
        stack.push(Segment(start, end));
        break;
      }
      case Id::Sym::Char: {
        // start=0-->ch=0-->end=0
        auto end = new Node;
        nodes.push_back(end);
        auto start = new Node({Edge::CharEdge(id.ch, end)});
        nodes.push_back(start);
        stack.push(Segment(start, end));
        break;
      }
      case Id::Sym::Concat: {
        //      | left | right |
        // start=0==>0-->0==>end=0
        Segment back(stack.top());
        stack.pop();
        Segment &front(stack.top());
        front.end->edges.push_back(Edge::EpsilonEdge(back.start));
        front.end = back.end;
        break;
      }
      case Id::Sym::Either: {
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
      case Id::Sym::More:
      case Id::Sym::LazyMore: {
        //       |-->0==>0-->|
        // start=0<--.<--.<--|   |-->end=0
        //       |-->.-->.-->.-->|
        Segment elem(stack.top());
        stack.pop();
        auto end = new Node;
        nodes.push_back(end);
        std::vector<Edge> edges;
        if (id.sym == Id::Sym::More) {
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
      case Id::Sym::Paren: {
        //         | elem  |
        // start=0-->0==>0-->end=0
        Segment elem(stack.top());
        stack.pop();
        auto end = new Node;
        nodes.push_back(end);
        auto start = new Node({Edge::StoreEdge(group_idx, elem.start)});
        nodes.push_back(start);
        elem.end->edges.push_back(Edge::StoreEndEdge(group_idx, end));
        ++group_idx;
        stack.push(Segment(start, end));
        break;
      }
      case Id::Sym::Quest:
      case Id::Sym::LazyQuest: {
        //       |   elem    |
        //       |-->0==>0-->|
        // start=0       end=0
        //       |-->.-->.-->|
        Segment elem(stack.top());
        stack.pop();
        auto end = new Node;
        nodes.push_back(end);
        std::vector<Edge> edges;
        if (id.sym == Id::Sym::Quest) {
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
//        assert(false);
        break;
      }
    }
  }
  assert(stack.size() == 1);
  Segment &seg(stack.top());
  seg.end->status = Node::Match;
  return Graph(seg, std::move(nodes));
}

int Graph::Match(const std::string &s) const {
  std::stack<std::pair<
      std::string::const_iterator, Node *>> stack;
  stack.push(std::make_pair(s.begin(), seg_.start));
  while (!stack.empty()) {
    auto[it, node] = stack.top();
    stack.pop();
    if (s.end() == it) {
      if (node->IsMatch())
        return it - s.begin();
      else
        continue;
    }
    for (auto edge = node->edges.rbegin(); edge != node->edges.rend(); ++edge) {
      std::string::const_iterator tit = it;
      bool match = true;
      switch (edge->type) {
        case Edge::Any: {
          ++tit;
          FallThrough;
        }
        case Edge::Epsilon: {
          break;
        }
        case Edge::Char: {
          if (*tit != edge->ch.val) {
            match = false;
            break;
          }
          ++tit;
          break;
        }
        case Edge::Store: {
          break;
        }
        default:break;
      }
//      if (!EdgeMatchStrIt(*edge, &tit)) continue;
      if (match)
        stack.push(std::make_pair(tit, edge->next));
    }
    if (Node::Match == node->status) return it - s.begin();
  }
  return -1;
}
int Graph::Match(const std::string &s, std::vector<std::string> *groups) const {
  struct Pos {
    std::string::const_iterator it;
    Node *node;
    size_t idx;

    Pos(std::string::const_iterator it, Node *node, size_t idx) :
        it(it), node(node), idx(idx) {}
  };
  std::stack<Pos> stack;
  Pos cur(s.begin(), seg_.start, 0);
  while (true) {
    auto &edge = cur.node->edges[cur.idx];
    bool match = true;
    switch (edge.type) {
      case Edge::Any: {
        ++cur.it;
        FallThrough;
      }
      case Edge::Epsilon: {
        break;
      }
      case Edge::Char: {
        if (*cur.it != edge.ch.val) {
          match = false;
          break;
        }
        ++cur.it;
        break;
      }
      case Edge::Store: {
        break;
      }
      default:break;
    }
    if (match) {
      auto *next = edge.next;
      if (next->IsMatch()) {
        return cur.it - s.begin();
      }
      if (!next->edges.empty()) {  // traverse the children
        stack.push(cur);
        cur = Pos(cur.it, next, 0);
      } else {
        assert(false);
      }
    } else {
      // go other children, or pop the parent node
      while (true) {
        if (++cur.idx < cur.node->edges.size()) break;
        if (stack.empty()) return -1;
        cur = stack.top();
        stack.pop();
      }
    }
  }
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
        printf("%d-->|%c|%d\n", map[node], edge.ch.val, map[edge.next]);
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
      if (**it == edge.ch.val) {
        ++*it;
        return true;
      } else {
        return false;
      }
    }
    default: assert(false);
      return false;
  }
}
void Graph::Deallocate() {
  for (Node *node : nodes_) {
    delete node;
  }
}

}  // namespace regex
