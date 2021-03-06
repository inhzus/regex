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
    case NegAhead:
      delete ahead.graph;
      break;
    case Brake:
      delete brake.pass;
      break;
    case Func:
      delete func.f;
      break;
    case Repeat:
      delete repeat.val;
      break;
    case Lower:
    case Upper:
      delete bound;
      break;
    case Set:
    case SetEx:
      delete set;
      break;
    default:
      break;
  }
}

namespace ch {
static constexpr const char kBackslash = '\\', kGroup = 'g', kAngle = '<',
                            kAngleEnd = '>';
}

std::string Matcher::Sub(std::string_view s) const {
  std::string sub;
  sub.reserve(s.size());
  auto it = s.begin();
  auto append_group = [&it = it, &sub = sub, this]() {
    size_t idx = 0;
    for (; *it >= '0' && *it <= '9'; ++it) {
      idx = idx * 10 + (*it - '0');
    }
    assert(idx < groups().size());
    sub.append(this->Group(idx));
  };
  while (it != s.end()) {
    if (*it != ch::kBackslash) {
      sub.push_back(*it++);
      continue;
    }
    if (*++it != ch::kGroup) {
      assert(*it >= '0' && *it <= '9');
      append_group();
      continue;
    }
    ++it;
    assert(*it == ch::kAngle);
    ++it;
    if (*it >= '0' && *it <= '9') {
      append_group();
      assert(*it == ch::kAngleEnd);
      ++it;
      continue;
    }
    auto left = it;
    for (; *it != ch::kAngleEnd; ++it) {
    }
    sub.append(Group(std::string_view(left, it - left)));
    ++it;
  }
  return sub;
}

#define FallThrough \
  do {              \
  } while (false)
Graph Graph::Compile(std::string_view s) { return Compile(Exp::FromStr(s)); }
Graph Graph::Compile(Exp &&exp) {
  std::stack<Segment> stack;
  std::vector<Node *> nodes;  // for memory management

  for (auto &id : exp.ids) {
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
        auto *sub_graph = new Graph(exp.group_num, seg.start, {}, {});
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
        auto start = new Node(Edge::FuncEdge(
            elem.start,
            new std::function<void()>(
                [b = elem.end->edges.back().brake.pass]() { *b = true; })));
        nodes.push_back(start);
        stack.push(Segment(start, end));
        break;
      }
      case Id::Sym::Begin: {
        auto end = new Node;
        nodes.push_back(end);
        auto start = new Node(Edge::BeginEdge(end));
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
      case Id::Sym::End: {
        auto end = new Node;
        nodes.push_back(end);
        auto start = new Node(Edge::EndEdge(end));
        nodes.push_back(start);
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
            start =
                new Node(Edge::EpsilonEdge(elem.start), Edge::EpsilonEdge(end));
            break;
          }
          case Id::Sym::RelMore: {
            start =
                new Node(Edge::EpsilonEdge(end), Edge::EpsilonEdge(elem.start));
            break;
          }
          default:  // ain't fall to default case, only to avoid clang warning
          case Id::Sym::PosMore: {
            std::function<void()> initializer;
            auto loop = new Node(Edge::EpsilonEdge(elem.start),
                                 Edge::BrakeEdge(end, new bool));
            nodes.push_back(loop);
            start = new Node(Edge::FuncEdge(
                loop, new std::function<void()>{
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
      case Id::Sym::Plus:
      case Id::Sym::PosPlus:
      case Id::Sym::RelPlus: {
        // special case as {1,}
        Segment elem(stack.top());
        stack.pop();
        auto end = new Node;
        nodes.push_back(end);
        auto repeat = new size_t;
        auto loop = new Node(Edge::EpsilonEdge(elem.start),
                             Edge::LowerEdge(end, repeat, 1));
        nodes.push_back(loop);
        elem.end->edges.push_back(Edge::RepeatEdge(loop, repeat));
        auto start = new Node(Edge::FuncEdge(
            loop, new std::function<void()>({[r = repeat]() { *r = 0; }})));
        nodes.push_back(start);
        switch (static_cast<int>(id.sym)) {
          case Id::Sym::Plus: {
            break;
          }
          case Id::Sym::RelPlus: {
            std::reverse(loop->edges.begin(), loop->edges.end());
            break;
          }
          case Id::Sym::PosPlus: {
            auto brake_end = new Node;
            nodes.push_back(brake_end);
            end->edges.push_back(Edge::BrakeEdge(brake_end, new bool));
            start = new Node(Edge::FuncEdge(
                start,
                new std::function<void()>(
                    [b = end->edges.back().brake.pass]() { *b = true; })));
            nodes.push_back(start);
            end = brake_end;
            break;
          }
        }
        stack.push(Segment(start, end));
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
            start =
                new Node(Edge::EpsilonEdge(elem.start), Edge::EpsilonEdge(end));
            elem.end->edges.push_back(Edge::EpsilonEdge(end));
            break;
          }
          case Id::Sym::RelQuest: {
            start =
                new Node(Edge::EpsilonEdge(end), Edge::EpsilonEdge(elem.start));
            elem.end->edges.push_back(Edge::EpsilonEdge(end));
            break;
          }
          default:
          case Id::Sym::PosQuest: {
            //                        |   elem    |
            //      |func-brake|      |-->0==>0-->|    |brake|
            // start=0-->.-->loop=0-->|           |-->0-->.-->end=0
            //                        |-->.-->.-->|
            auto loop =
                new Node(Edge::EpsilonEdge(elem.start), Edge::EpsilonEdge(end));
            nodes.push_back(loop);
            auto brake_end = new Node;
            nodes.push_back(brake_end);
            end->edges.push_back(Edge::BrakeEdge(brake_end, new bool));
            start = new Node(Edge::FuncEdge(
                loop, new std::function<void()>{
                          [b = end->edges[0].brake.pass]() { *b = true; }}));
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
        elem.end->edges.push_back(Edge::RepeatEdge(loop, repeat));
        auto start = new Node(Edge::FuncEdge(
            loop, new std::function<void()>{[r = repeat]() { *r = 0; }}));
        nodes.push_back(start);
        if (id.repeat->upper != std::numeric_limits<size_t>::max()) {
          loop->edges.push_back(
              Edge::UpperEdge(elem.start, repeat, id.repeat->upper));
        } else {
          loop->edges.push_back(Edge::EpsilonEdge(elem.start));
        }
        auto end = new Node;
        nodes.push_back(end);
        if (id.repeat->lower != 0) {
          loop->edges.push_back(Edge::LowerEdge(end, repeat, id.repeat->lower));
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
            start = new Node(Edge::FuncEdge(
                start, new std::function<void()>(
                           [b = end->edges[0].brake.pass]() { *b = true; })));
            nodes.push_back(start);
            end = brake_end;
            break;
          }
        }
        stack.push(Segment(start, end));
        break;
      }
      case Id::Sym::Set: {
        //          set
        // start=0-->.-->end=0
        auto end = new Node;
        nodes.push_back(end);
        auto start = new Node(Edge::SetEdge(end, std::move(id.set->val)));
        nodes.push_back(start);
        stack.push(Segment(start, end));
        break;
      }
      case Id::Sym::SetEx: {
        //          set
        // start=0-->.-->end=0
        auto end = new Node;
        nodes.push_back(end);
        auto start = new Node(Edge::SetExEdge(end, std::move(id.set->val)));
        nodes.push_back(start);
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
  Node *end = new Node;
  end->status = Node::Match;
  seg.end->edges.push_back(Edge::MatchEdge(end));
  nodes.push_back(end);
  Graph graph(exp.group_num, seg.start, std::move(nodes),
              std::move(exp.named_group));
  return graph;
}

int Graph::MatchLen(std::string_view s) const {
  std::vector<std::string_view> groups;
  if (!MatchGroups(s, &groups)) return -1;
  return groups[0].size();
}
bool Graph::MatchGroups(std::string_view s,
                        std::vector<std::string_view> *groups) const {
  Matcher matcher = Match(s);
  if (groups != nullptr) *groups = matcher.groups_;
  return matcher.ok();
}

void Graph::Match(std::string_view s, Matcher *matcher) const {
  struct Pos {
    std::string_view::const_iterator it;
    Node *node;
    size_t idx;

    Pos(std::string_view::const_iterator it, Node *node, size_t idx)
        : it(it), node(node), idx(idx) {}
  };

  std::stack<Pos> stack;
  std::vector<std::pair<std::string_view::const_iterator,
                        std::string_view::const_iterator>>
      boundary;
  auto start = s.begin();
  do {
    Pos cur(start, start_, 0);
    boundary.assign(group_num_, {cur.it, cur.it});

    while (true) {
      // set backtrack false
      // check edge matched. if not, set backtrack true
      // if backtrack
      //   do stack backtracking
      // else
      //   go dig children
      bool backtrack = false;
      auto &edge = cur.node->edges[cur.idx];
      switch (edge.type) {
        case Edge::Any:
        case Edge::Char:
        case Edge::Set:
        case Edge::SetEx: {
          if (cur.it == s.end()) {
            backtrack = true;
          }
          break;
        }
        case Edge::Ref: {
          auto &pair = boundary[edge.ref.idx];
          if (pair.second - pair.first > s.end() - cur.it) {
            backtrack = true;
          }
          break;
        }
        default:
          break;
      }
      if (!backtrack) {
        switch (edge.type) {
          case Edge::Ahead: {
            edge.ahead.graph->Match(std::string_view(cur.it, s.end() - cur.it),
                                    matcher);
            if (!matcher->ok()) {
              backtrack = true;
            }
            break;
          }
          case Edge::NegAhead: {
            edge.ahead.graph->Match(std::string_view(cur.it, s.end() - cur.it),
                                    matcher);
            if (matcher->ok()) {
              backtrack = true;
            }
            break;
          }
          case Edge::Any: {
            ++cur.it;
            FallThrough;
          }
          case Edge::Epsilon: {
            break;
          }
          case Edge::Begin: {
            if (cur.it != s.begin()) {
              backtrack = true;
            }
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
            if (*cur.it != edge.ch.val) {
              backtrack = true;
              break;
            }
            ++cur.it;
            break;
          }
          case Edge::End: {
            if (cur.it != s.end()) {
              backtrack = true;
            }
            break;
          }
          case Edge::Func: {
            (*edge.func.f)();
            break;
          }
          case Edge::Lower: {
            if (*edge.bound->repeat < edge.bound->num) {
              backtrack = true;
            }
            break;
          }
          case Edge::Match: {
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
          case Edge::Set: {
            if (edge.set->val.Contains(*cur.it)) {
              ++cur.it;
            } else {
              backtrack = true;
            }
            break;
          }
          case Edge::SetEx: {
            if (edge.set->val.Contains(*cur.it)) {
              backtrack = true;
            } else {
              ++cur.it;
            }
            break;
          }
          case Edge::Upper: {
            if (*edge.bound->repeat >= edge.bound->num) {
              backtrack = true;
            }
            break;
          }
          default:
            break;
        }
      }
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
    if (!matcher->ok()) continue;
    //    groups->clear();  // to collect groups in look-ahead sub-graph
    for (size_t i = 0; i < boundary.size(); ++i) {
      if (boundary[i].first >= boundary[i].second) continue;
      matcher->groups_[i] = std::string_view(
          boundary[i].first, boundary[i].second - boundary[i].first);
    }
    return;
  } while (start++ != s.end());
}

Matcher Graph::Match(std::string_view s) const {
  Matcher matcher(s, group_num_, named_group_);
  Match(s, &matcher);
  return matcher;
}

std::string Graph::Sub(std::string_view sub, std::string_view s) const {
  std::string ret;
  while (true) {
    auto matcher = Match(s);
    if (!matcher.ok()) {
      ret.append(s);
      return ret;
    }
    ret.append(s.substr(0, matcher.BeginIdx()));
    ret.append(matcher.Sub(sub));
    s = s.substr(matcher.EndIdx());
  }
}

void Graph::DrawMermaid() const {
  int id = 0;
  std::unordered_map<Node *, int> map{{start_, id++}};
  std::stack<Node *> stack;
  stack.push(start_);
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
        case Edge::Ahead:
          s = "?=";
          break;
        case Edge::NegAhead:
          s = "?!";
          break;
        case Edge::Any:
          s = "any";
          break;
        case Edge::Begin:
          s = "begin";
          break;
        case Edge::Brake:
          s = "brake";
          break;
        case Edge::Char:
          s = "char: " + std::string(1, edge.ch.val);
          break;
        case Edge::End:
          s = "end";
          break;
        case Edge::Func:
          s = "func";
          break;
        case Edge::Lower:
          s = "lower: " + std::to_string(edge.bound->num);
          break;
        case Edge::Match:
          s = "match";
          break;
        case Edge::Named:
          s = "<" + std::to_string(edge.named.idx);
          break;
        case Edge::NamedEnd:
          s = std::to_string(edge.named_end.idx) + ">";
          break;
        case Edge::Ref:
          s = "<" + std::to_string(edge.ref.idx) + ">";
          break;
        case Edge::Store:
          s = "(" + std::to_string(edge.store.idx);
          break;
        case Edge::StoreEnd:
          s = std::to_string(edge.store.idx) + ")";
          break;
        case Edge::Repeat:
          s = "repeat";
          break;
        case Edge::Set:
          s = std::string(1, '[') +
              std::to_string(edge.set->val.pos.ranges.size()) + ']';
          break;
        case Edge::SetEx:
          s = std::string("[^") +
              std::to_string(edge.set->val.pos.ranges.size()) + ']';
          break;
        case Edge::Upper:
          s = "upper: " + std::to_string(edge.bound->num);
          break;
        default:
          break;
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
  printf("\n");
}

void Graph::Deallocate() {
  for (Node *node : nodes_) {
    delete node;
  }
}

}  // namespace regex
