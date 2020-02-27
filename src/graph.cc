//
// Copyright [2020] <inhzus>
//

#include "regex/graph.h"

#include <cassert>
#include <functional>
#include <queue>
#include <stack>
#include <unordered_map>

namespace regex {

Edge::Edge(Edge &&e) noexcept : type(e.type), next(e.next), named(e.named) {
  e.type = Empty;
  e.named = {};
}

Edge::~Edge() {
  switch (type) {
    case Ahead:
    case NegAhead: delete ahead.graph;
      break;
    case Brake: delete brake.pass;
      break;
    case Named:
    case NamedEnd: delete named.name;
      break;
    case Repeat: delete bound.repeat;
      break;
    default: break;
  }
}

bool Node::WillMatch() const {
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
            Edge::EpsilonEdge(left.start),
            Edge::EpsilonEdge(right.start));
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
        Node *start;
        if (greedy) {
          start = new Node(Edge::EpsilonEdge(elem.start),
                           Edge::EpsilonEdge(end));
        } else {
          start = new Node(Edge::EpsilonEdge(end),
                           Edge::EpsilonEdge(elem.start));
        }
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
        Node *start;
        if (greedy) {
          start = new Node(Edge::EpsilonEdge(elem.start),
                           Edge::EpsilonEdge(end));
        } else {
          start = new Node(Edge::EpsilonEdge(end),
                           Edge::EpsilonEdge(elem.start));
        }
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
        start->edges.push_back(Edge::CharEdge(end, ch));
        stack.push(Segment(start, end));
        break;
      }
    }
  }
  assert(stack.size() == 1);
  Segment &seg(stack.top());
  seg.end->status = Node::Match;
  return Graph(seg, std::move(nodes), {}, 1);
}
Graph Graph::Compile(const std::string &s) {
  return Compile(Exp::FromStr(s));
}
Graph Graph::Compile(Exp &&exp) {
  std::stack<Segment> stack;
  std::vector<Node *> nodes;  // for memory management
  std::vector<std::function<void()>> initializers;

  for (auto id : exp.ids) {
    switch (static_cast<int>(id.sym)) {
      case Id::Sym::AheadPr:
      case Id::Sym::NegAheadPr: {
        //        {sub-graph}
        //             |
        // start=0-->ahead-->end=0
        // Build a sub-graph with segment at the top of stack.
        // Memory management of nodes still belongs to the parent graph.
        // Need group_num to assign matched groups in the sub-graph to the
        // groups passed to the parent graph.
        Segment seg(stack.top());
        stack.pop();
        seg.end->status = Node::Match;
        auto *sub_graph = new Graph(seg, {}, {}, exp.group_num);
        auto end = new Node;
        nodes.push_back(end);
        Node *start;
        if (id.sym == Id::Sym::AheadPr) {
          start = new Node(Edge::AheadEdge(end, sub_graph));
        } else {
          // as id.sym == Id::Sym::NegAheadPr
          start = new Node(Edge::NegAheadEdge(end, sub_graph));
        }
        nodes.push_back(start);
        stack.push(Segment(start, end));
        break;
      }
      case Id::Sym::Any: {
        // start=0-->any-->end=0
        auto end = new Node;
        nodes.push_back(end);
        auto start = new Node(Edge::AnyEdge(end));
        nodes.push_back(start);
        stack.push(Segment(start, end));
        break;
      }
      case Id::Sym::Char: {
        // start=0-->ch=0-->end=0
        auto end = new Node;
        nodes.push_back(end);
        auto start = new Node(Edge::CharEdge(end, id.ch));
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
        auto start = new Node(Edge::EpsilonEdge(left.start),
                              Edge::EpsilonEdge(right.start));
        nodes.push_back(start);
        auto end = new Node;
        nodes.push_back(end);
        left.end->edges.push_back(Edge::EpsilonEdge(end));
        right.end->edges.push_back(Edge::EpsilonEdge(end));
        stack.push(Segment(start, end));
        break;
      }
      case Id::Sym::More:
      case Id::Sym::PosMore:
      case Id::Sym::RelMore: {
        //       |-->0==>0-->|
        // start=0<--.<--.<--|   |-->end=0
        //       |-->.-->.-->.-->|
        Segment elem(stack.top());
        stack.pop();
        auto end = new Node;
        nodes.push_back(end);
        std::vector<Edge> edges;
        Node *start;
        switch (static_cast<int>(id.sym)) {
          case Id::Sym::More: {
            start = new Node(Edge::EpsilonEdge(elem.start),
                             Edge::EpsilonEdge(end));
            break;
          }
          case Id::Sym::RelMore: {
            start = new Node(Edge::EpsilonEdge(end),
                             Edge::EpsilonEdge(elem.start));
            break;
          }
          default:  // ain't fall to default case, only to avoid clang warning
          case Id::Sym::PosMore: {
            std::function<void()> initializer;
            start = new Node(
                Edge::EpsilonEdge(elem.start),
                Edge::BrakeEdge(end, new bool, &initializer));
            initializers.push_back(std::move(initializer));
          }
        }
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
        auto start = new Node(Edge::StoreEdge(elem.start, id.store.idx));
        nodes.push_back(start);
        elem.end->edges.push_back(Edge::StoreEndEdge(end, id.store.idx));
        stack.push(Segment(start, end));
        break;
      }
      case Id::Sym::UnParen: {
        break;
      }
      case Id::Sym::Quest:
      case Id::Sym::RelQuest: {
        //       |   elem    |
        //       |-->0==>0-->|
        // start=0       end=0
        //       |-->.-->.-->|
        Segment elem(stack.top());
        stack.pop();
        auto end = new Node;
        nodes.push_back(end);
        Node *start;
        switch (static_cast<int>(id.sym)) {
          case Id::Sym::Quest: {
            start = new Node(Edge::EpsilonEdge(elem.start),
                             Edge::EpsilonEdge(end));
            break;
          }
          case Id::Sym::RelQuest: {
            start = new Node(Edge::EpsilonEdge(end),
                             Edge::EpsilonEdge(elem.start));
            break;
          }
          default:
          case Id::Sym::PosQuest: {
            std::function<void()> initializer;
            start = new Node(
                Edge::EpsilonEdge(elem.start),
                Edge::BrakeEdge(end, new bool, &initializer));
            initializers.push_back(std::move(initializer));
            break;
          }
        }
        nodes.push_back(start);
        elem.end->edges.push_back(Edge::EpsilonEdge(end));
        stack.push(Segment(start, end));
        break;
      }
      case Id::Sym::Repeat: {
        //       Upper
        //       |-->0==>0-->|
        //       |  Repeat   |
        // start=0<--.<--.<--|
        //       |  Lower        |-->end=0
        //       |-->.-->.-->.-->|
        Segment elem(stack.top());
        stack.pop();
        auto start = new Node;
        nodes.push_back(start);
        auto repeat = new size_t;
        std::function<void()> initializer;
        elem.end->edges.push_back(
            Edge::RepeatEdge(start, repeat, &initializer));
        initializers.push_back(std::move(initializer));
        if (id.repeat.upper != std::numeric_limits<size_t>::max()) {
          start->edges.push_back(
              Edge::UpperEdge(elem.start, repeat, id.repeat.upper));
        } else {
          start->edges.push_back(Edge::EpsilonEdge(elem.start));
        }
        auto end = new Node;
        nodes.push_back(end);
        if (id.repeat.lower != 0) {
          start->edges.push_back(Edge::LowerEdge(end, repeat, id.repeat.lower));
        } else {
          start->edges.push_back(Edge::EpsilonEdge(end));
        }
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
  return Graph(seg, std::move(nodes), std::move(initializers), exp.group_num);
}

int Graph::Match(const std::string &s) const {
  std::vector<std::string> groups;
  if (!Match(s, &groups)) return -1;
  return groups[0].size();
}
int Graph::Match(const std::string &s, std::vector<std::string> *groups) const {
  for (const auto &f : initializers_) { f(); }
  struct Pos {
    std::string::const_iterator it;
    Node *node;
    size_t idx;

    Pos(std::string::const_iterator it, Node *node, size_t idx) :
        it(it), node(node), idx(idx) {}
  };

  std::stack<Pos> stack;
  Pos cur(s.begin(), seg_.start, 0);
  std::vector<std::pair<
      std::string::const_iterator, std::string::const_iterator>>
      boundary(group_num_, {cur.it, cur.it});
  bool match_str;

  while (true) {
    // set backtrack false
    // if reach str end, then
    //   if node will match, then
    //     match and return
    //   else
    //     set backtrack true
    // else
    //   check edge matched. if not, set backtrack true
    // if backtrack
    //   do stack backtracking
    // else
    //   go dig children
    bool backtrack = false;
    auto &edge = cur.node->edges[cur.idx];
//    if (cur.it == s.end()) {
//      if (cur.node->WillMatch()) {
//        boundary[0].second = s.end();
//        match_str = true;
//        goto finally;
//      } else {
//        backtrack = true;
//      }
//    } else {
    switch (edge.type) {
      case Edge::Ahead: {
        if (!edge.ahead.graph->Match(
            std::string(cur.it, s.end()), groups)) {
          backtrack = true;
        }
        break;
      }
      case Edge::NegAhead: {
        if (edge.neg_ahead.graph->Match(
            std::string(cur.it, s.end()), groups)) {
          backtrack = true;
        }
        break;
      }
      case Edge::Any: {
        if (cur.it == s.end()) {
          backtrack = true;
          break;
        }
        ++cur.it;
        FallThrough;
      }
      case Edge::Epsilon: {
        break;
      }
      case Edge::Brake: {
        if (*edge.brake.pass) {
          *edge.brake.pass = false;
        } else {
          backtrack = true;
        }
        break;
      }
      case Edge::Char: {
        if (cur.it == s.end() || *cur.it != edge.ch.val) {
          backtrack = true;
          break;
        }
        ++cur.it;
        break;
      }
      case Edge::Lower: {
        if (*edge.bound.repeat < edge.bound.num) {
          backtrack = true;
        }
        break;
      }
      case Edge::Store: {
        boundary[edge.store.idx].first = cur.it;
        break;
      }
      case Edge::StoreEnd: {
        boundary[edge.store_end.idx].second = cur.it;
        break;
      }
      case Edge::Repeat: {
        ++*edge.repeat.val;
        break;
      }
      case Edge::Upper: {
        if (*edge.bound.repeat >= edge.bound.num) {
          backtrack = true;
        }
        break;
      }
      default:break;
    }
//    }
    if (backtrack) {
      // go other children, or pop the parent node
      while (true) {
        if (++cur.idx < cur.node->edges.size()) break;
        if (stack.empty()) {
          match_str = false;
          goto finally;
        }
        cur = stack.top();
        stack.pop();
      }
    } else {
      auto *next = edge.next;
      if (next->status == Node::Match) {
        match_str = true;
        boundary[0].second = cur.it;
        goto finally;
      }
      if (!next->edges.empty()) {  // traverse the children
        stack.push(cur);
        cur = Pos(cur.it, next, 0);
      } else {
        assert(false);
      }
    }
  }
  finally:
  if (!match_str) return false;
  if (groups) {
//    groups->clear();  // to collect groups in look-ahead sub-graph
    groups->resize(group_num_);
    for (size_t i = 0; i < boundary.size(); ++i) {
      if (boundary[i].first >= boundary[i].second) continue;
      (*groups)[i].assign(boundary[i].first, boundary[i].second);
    }
  }
  return true;
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
      std::string s;
      switch (edge.type) {
        case Edge::Ahead:s = "?=";
          break;
        case Edge::NegAhead: s = "?!";
          break;
        case Edge::Any: s = ".";
          break;
        case Edge::Brake: s = "+";
          break;
        case Edge::Char: s = edge.ch.val;
          break;
        case Edge::Store: s = "(" + std::to_string(edge.store.idx);
          break;
        case Edge::StoreEnd: s = std::to_string(edge.store.idx) + ")";
          break;
        default: break;
      }
      if (edge.type == Edge::Epsilon) {
        printf("%d-->%d\n", map[node], map[edge.next]);
      } else {
        printf("%d-->|%s|%d\n", map[node], s.c_str(), map[edge.next]);
      }
      if (Node::Match == edge.next->status) {
        printf("%d-->|match|%d\n", map[edge.next], map[edge.next]);
      }
    }
  }
}

void Graph::Deallocate() {
  for (Node *node : nodes_) {
    delete node;
  }
}

}  // namespace regex
