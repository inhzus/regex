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

const char *Matcher::kEnd = nullptr;

Edge::Edge(Edge &&e) noexcept : type(e.type), next(e.next), bound(e.bound) {
  e.type = Empty;
  e.bound = {};
}

Edge &Edge::operator=(Edge &&e) noexcept {
  type = e.type;
  next = e.next;
  named = e.named;
  e.type = Empty;
  return *this;
}

Edge::~Edge() {
  switch (type) {
    case Ahead:
    case NegAhead: delete ahead.graph;
      break;
    case Brake: delete brake.pass;
      break;
    case Func: delete func.f;
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
      case ch::kMore:
      case ch::kPlus:
      case ch::kQuest: {
        // lazy mode: *?, +?, ??
        auto next(sit + 1);
        if (next != s.end() && *next == ch::kQuest) {
          ++sit;
          greedy = false;
        }
        break;
      }
      default: break;
    }
    switch (ch) {
      case ch::kConcat: {
        //      | left | right |
        // start=0==>0-->0==>end=0
        Segment back(stack.top());
        stack.pop();
        Segment &front(stack.top());
        front.end->edges.push_back(Edge::EpsilonEdge(back.start));
        front.end = back.end;
        break;
      }
      case ch::kEither: {
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
      case ch::kMore: {
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
      case ch::kPlus: {
        break;
      }
      case ch::kQuest: {
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
  return Graph(1, seg, std::move(nodes), {});
}
Graph Graph::Compile(const std::string &s) {
  return Compile(Exp::FromStr(s));
}
Graph Graph::Compile(Exp &&exp) {
  std::stack<Segment> stack;
  std::vector<Node *> nodes;  // for memory management

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
        auto *sub_graph = new Graph(exp.group_num, seg, {}, exp.named_group);
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
      case Id::Sym::AtomicPr: {
        //   |func-brake|     |brake|
        // start=0-->.-->0==>0-->.-->end=0
        //              |elem|
        Segment elem(stack.top());
        stack.pop();
        auto end = new Node;
        nodes.push_back(end);
        elem.end->edges.push_back(Edge::BrakeEdge(end, new bool));
        auto start = new Node(
            Edge::FuncEdge(elem.start, new std::function<void()>(
                [b = elem.end->edges.back().brake.pass]() { *b = true; })));
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
            auto loop = new Node(
                Edge::EpsilonEdge(elem.start),
                Edge::BrakeEdge(end, new bool));
            nodes.push_back(loop);
            start = new Node(
                Edge::FuncEdge(loop, new std::function<void()>{
                    [b = loop->edges[1].brake.pass]() { *b = true; }}));
          }
        }
        nodes.push_back(start);
        elem.end->edges.push_back(Edge::EpsilonEdge(start));
        stack.push(Segment(start, end));
        break;
      }
      case Id::Sym::NamedPr: {
        Segment elem(stack.top());
        stack.pop();
        auto end = new Node;
        nodes.push_back(end);
        auto start = new Node(Edge::NamedEdge(elem.start, id.named.idx));
        nodes.push_back(start);
        elem.end->edges.push_back(Edge::NamedEndEdge(end, id.named.idx));
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
      case Id::Sym::PosQuest:
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
            elem.end->edges.push_back(Edge::EpsilonEdge(end));
            break;
          }
          case Id::Sym::RelQuest: {
            start = new Node(Edge::EpsilonEdge(end),
                             Edge::EpsilonEdge(elem.start));
            elem.end->edges.push_back(Edge::EpsilonEdge(end));
            break;
          }
          default:
          case Id::Sym::PosQuest: {
            //                        |   elem    |
            //      |func-brake|      |-->0==>0-->|    |brake|
            // start=0-->.-->loop=0-->|           |-->0-->.-->end=0
            //                        |-->.-->.-->|
            auto loop = new Node(
                Edge::EpsilonEdge(elem.start),
                Edge::EpsilonEdge(end));
            auto brake_end = new Node;
            nodes.push_back(brake_end);
            end->edges.push_back(Edge::BrakeEdge(brake_end, new bool));
            start = new Node(
                Edge::FuncEdge(loop, new std::function<void()>{
                    [b = end->edges[0].brake.pass]() { *b = true; }
                }));
            elem.end->edges.push_back(Edge::EpsilonEdge(end));
            end = brake_end;
            break;
          }
        }
        nodes.push_back(start);
        stack.push(Segment(start, end));
        break;
      }
      case Id::Sym::RefPr: {
        //          | ref |
        // start=0-->0-->0-->end=0
        auto end = new Node;
        nodes.push_back(end);
        auto start = new Node(Edge::RefEdge(end, id.ref.idx));
        nodes.push_back(start);
        stack.push(Segment(start, end));
        break;
      }
      case Id::Sym::Repeat:
      case Id::Sym::PosRepeat:
      case Id::Sym::RelRepeat: {
        //                        Upper
        //                    |-->0==>0-->|
        //      |func-repeat| |  Repeat   |
        // start=0-->.-->loop=0<--.<--.<--|
        //                    |  Lower        |-->end=0
        //                    |-->.-->.-->.-->|
        Segment elem(stack.top());
        stack.pop();
        auto loop = new Node;
        nodes.push_back(loop);
        auto repeat = new size_t;
        elem.end->edges.push_back(
            Edge::RepeatEdge(loop, repeat));
        auto start = new Node(
            Edge::FuncEdge(loop, new std::function<void()>{
                [r = repeat]() { *r = 0; }}));
        nodes.push_back(start);
        if (id.repeat.upper != std::numeric_limits<size_t>::max()) {
          loop->edges.push_back(
              Edge::UpperEdge(elem.start, repeat, id.repeat.upper));
        } else {
          loop->edges.push_back(Edge::EpsilonEdge(elem.start));
        }
        auto end = new Node;
        nodes.push_back(end);
        if (id.repeat.lower != 0) {
          loop->edges.push_back(Edge::LowerEdge(end, repeat, id.repeat.lower));
        } else {
          loop->edges.push_back(Edge::EpsilonEdge(end));
        }
        switch (static_cast<int>(id.sym)) {
          default:
          case Id::Sym::Repeat: {
            break;
          }
          case Id::Sym::RelRepeat: {
            std::reverse(loop->edges.begin(), loop->edges.end());
            break;
          }
          case Id::Sym::PosRepeat: {
            //                    Upper
            //                     |-->0==>0-->|
            //       |func-repeat| |  Repeat   |
            // start=0-->=0-->loop=0<--.<--.<--|              brake
            //   /func-brake|      |  Lower        |-->end=0-->|
            //                     |-->.-->.-->.-->|           0=brake_end
            auto brake_end = new Node;
            nodes.push_back(brake_end);
            end->edges.push_back(Edge::BrakeEdge(brake_end, new bool));
            start = new Node(
                Edge::FuncEdge(start, new std::function<void()>(
                    [b = end->edges[0].brake.pass]() { *b = true; })));
            end = brake_end;
            break;
          }
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
  return Graph(exp.group_num, seg, std::move(nodes),
               std::move(exp.named_group));
}

int Graph::MatchLen(const std::string &s) const {
  std::vector<std::string> groups;
  if (!MatchGroups(s, &groups)) return -1;
  return groups[0].size();
}
bool Graph::MatchGroups(
    const std::string &s, std::vector<std::string> *groups) const {
  Matcher matcher = Match(s);
  if (groups != nullptr)
    *groups = matcher.groups_;
  return matcher.ok();
}

void Graph::Match(const std::string &s, Matcher *matcher) const {
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
    switch (edge.type) {
      case Edge::Ahead: {
        edge.ahead.graph->Match(std::string(cur.it, s.end()), matcher);
        if (!matcher->ok()) {
          backtrack = true;
        }
        break;
      }
      case Edge::NegAhead: {
        edge.ahead.graph->Match(std::string(cur.it, s.end()), matcher);
        if (matcher->ok()) {
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
      case Edge::Func: {
        (*edge.func.f)();
        break;
      }
      case Edge::Lower: {
        if (*edge.bound.repeat < edge.bound.num) {
          backtrack = true;
        }
        break;
      }
      case Edge::Named: {
        boundary[edge.named.idx].first = cur.it;
        break;
      }
      case Edge::NamedEnd: {
        boundary[edge.named_end.idx].second = cur.it;
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
      case Edge::Ref: {
        auto &pair = boundary[edge.ref.idx];
        std::string_view view(&*pair.first, pair.second - pair.first);
        auto p = cur.it;
        auto vit = view.begin();
        for (; vit != view.end(); ++vit, ++p) {
          if (*p != *vit) {
            backtrack = true;
            break;
          }
        }
        if (!backtrack) {
          assert(vit == view.end());
          cur.it = p;
        }
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
          matcher->ok_ = false;
          goto finally;
        }
        cur = stack.top();
        stack.pop();
      }
    } else {
      auto *next = edge.next;
      if (next->status == Node::Match) {
        matcher->ok_ = true;
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
  if (!matcher->ok()) return;
//    groups->clear();  // to collect groups in look-ahead sub-graph
  for (size_t i = 0; i < boundary.size(); ++i) {
    if (boundary[i].first >= boundary[i].second) continue;
    matcher->groups_[i].assign(boundary[i].first, boundary[i].second);
  }
}

Matcher Graph::Match(const std::string &s) const {
  Matcher matcher(group_num_, named_group_);
  Match(s, &matcher);
  return matcher;
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
        case Edge::Any: s = "any";
          break;
        case Edge::Brake: s = "brake";
          break;
        case Edge::Char: s = "char: " + std::string(1, edge.ch.val);
          break;
        case Edge::Func: s = "func";
          break;
        case Edge::Lower: s = "lower: " + std::to_string(edge.bound.num);
          break;
        case Edge::Named: s = "<" + std::to_string(edge.named.idx);
          break;
        case Edge::NamedEnd: s = std::to_string(edge.named_end.idx) + ">";
          break;
        case Edge::Ref: s = "<" + std::to_string(edge.ref.idx) + ">";
          break;
        case Edge::Store: s = "(" + std::to_string(edge.store.idx);
          break;
        case Edge::StoreEnd: s = std::to_string(edge.store.idx) + ")";
          break;
        case Edge::Repeat: s = "repeat";
          break;
        case Edge::Upper: s = "upper: " + std::to_string(edge.bound.num);
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
