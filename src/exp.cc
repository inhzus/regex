//
// Copyright [2020] <inhzus>
//

#include "regex/exp.h"

#include <cassert>

namespace regex {

// the order of precedence for operators:
// 1. collation-related bracket symbols [==], [::], [..]
// 2. Escape characters "\"
// 3. Character set (bracket expression) []
// 4. Grouping ()
// 5. Single-character-ERE duplication * + ? {m,n}
// 6. Concatenation
// 7. Anchoring ^$
// 8. Alternation |
size_t Id::Sym::Order() const {
  switch (inner_) {
    case More:
    case LazyMore:
    case Quest:
    case LazyQuest:return 5;
    case Concat:return 6;
    case Either:return 8;
    default:assert(false);
      break;
  }
  return 0;
}

#define FALL_THROUGH do {} while (0)
std::vector<Id> Exp::Post() const {
  std::vector<Id> vector;
  std::string::const_iterator it(string_.begin());
  std::stack<Id> stack;
  Id last_id(Id::Sym::Begin);  // for concatenate characters
  auto pop_stack = [&vector = vector, &stack = stack, &id = last_id]() {
    while (!stack.empty()) {
      Id top = stack.top();
      if (top.sym == Id::Sym::LeftParen) break;
      if (top.sym.Order() >= id.sym.Order()) {
        stack.pop();
        vector.push_back(top);
      }
    }
  };
  for (;; ++it) {
    if (string_.end() == it) break;
    char ch = *it;
    bool greedy = true;
    switch (ch) {
      // set greedy or lazy mode
      case Char::kMore:
      case Char::kQuest: {
        auto next(it + 1);
        if (next != string_.end() && *next == Char::kQuest) {
          it = next;
          greedy = false;
        }
        break;
      }
      default: break;
    }
    switch (ch) {
      case Char::kAny: {
        last_id = Id(Id::Sym::Any);
        vector.push_back(last_id);
        break;
      }
      case Char::kEither: {
        last_id = Id(Id::Sym::Either);
        pop_stack();
        break;
      }
      case Char::kLeftParen: {
        last_id = Id(Id::Sym::LeftParen);
        stack.push(last_id);
        break;
      }
      case Char::kRightParen: {
        last_id = Id(Id::Sym::RightParen);
        while (true) {
          assert(!stack.empty());
          Id id = stack.top();
          stack.pop();
          if (id.sym == Id::Sym::LeftParen) {
            break;
          }
          vector.push_back(id);
        }
        break;
      }
      case Char::kMore: {
        if (greedy) {
          last_id = Id(Id::Sym::More);
        } else {
          last_id = Id(Id::Sym::LazyMore);
        }
        pop_stack();
        break;
      }
      case Char::kQuest : {
        if (greedy) {
          last_id = Id(Id::Sym::Quest);
        } else {
          last_id = Id(Id::Sym::LazyQuest);
        }
        pop_stack();
        break;
      }
      case Char::kBackslash: {
        // attention to order of precedence for regex operators
        ch = *++it;
        FALL_THROUGH;
      }
      default: {
        vector.emplace_back(ch);
        if (last_id.sym.IsOperand()) {
          stack.push(Id(Id::Sym::Concat));
        }
        last_id = Id(ch);
        break;
      }
    }
  }
  while (!stack.empty()) {
    vector.push_back(stack.top());
    stack.pop();
  }
  return vector;
}

}  // namespace regex
