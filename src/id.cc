//
// Copyright [2020] <inhzus>
//

#include "regex/id.h"

#include <cassert>

namespace regex {

// the order of precedence for operators (from high to low):
// 1. collation-related bracket symbols [==], [::], [..]
// 2. Escape characters "\"
// 3. Character set (bracket expression) []
// 4. Grouping ()
// 5. Single-character-ERE duplication * + ? {m,n}
// 6. Concatenation
// 7. Anchoring ^$
// 8. Alternation |
size_t Id::Sym::Order(Id::Sym::_Inner inner) {
  switch (inner) {
//    case Paren:
//    case ParenEnd:return 4;
    case More:
    case LazyMore:
    case Quest:
    case LazyQuest:return 5;
    case Concat:return 6;
    case Either:return 8;
    default:return 0;
  }
}

#define FALL_THROUGH do {} while (0)
std::vector<Id> StrToPostfixIds(const std::string &s) {
  std::vector<Id> vector;
  std::string::const_iterator it(s.begin());
  std::stack<Id, std::vector<Id>> stack;
  std::stack<bool, std::vector<bool>> concat_stack;
  concat_stack.push(false);
  size_t store_idx = 1;
  auto push_operator = [&vector = vector, &stack = stack](Id id) {
    switch (static_cast<int>(id.sym)) {
      // directly output the unary operator
      case Id::Sym::More:
      case Id::Sym::LazyMore:
      case Id::Sym::Quest:
      case Id::Sym::LazyQuest:vector.push_back(id);
        return;
    }
    while (!stack.empty()) {
      Id top = stack.top();
      if (top.sym.IsParen()) break;
      if (top.sym.order() <= id.sym.order()) {
        stack.pop();
        vector.push_back(top);
      } else {
        break;
      }
    }
    stack.push(id);
  };
  for (; s.end() != it; ++it) {
    char op = *it;
    bool greedy = true;
    switch (op) {
      // set greedy or lazy mode
      case Char::kMore:
      case Char::kQuest: {
        auto next(it + 1);
        if (next != s.end() && *next == Char::kQuest) {
          it = next;
          greedy = false;
        }
        break;
      }
      default: break;
    }
    switch (op) {
      case Char::kAny: {
        vector.emplace_back(Id::Sym::Any);
        break;
      }
      case Char::kEither: {
        push_operator(Id(Id::Sym::Either));
        break;
      }
      case Char::kParen: {
        auto quest(it + 1);
        if (quest == s.end() || *quest != Char::kParenFLag) {
          Id id(Id::Sym::Paren);
          id.store.idx = store_idx++;
          stack.push(id);
          break;
        }
        auto flag(quest + 1);
        if (flag == s.end()) break;  // grammar error
        switch (*flag) {
          case Char::kUnParenFlag: {
            stack.push(Id(Id::Sym::UnParen));
            break;
          }
          default: break;
        }
        it = flag;
        break;
      }
      case Char::kParenEnd: {
        while (true) {
          assert(!stack.empty());
          Id id = stack.top();
          stack.pop();
          vector.push_back(id);
          if (id.sym.IsParen()) {
            break;
          }
        }
        break;
      }
      case Char::kMore: {
        if (greedy) {
          push_operator(Id(Id::Sym::More));
        } else {
          push_operator(Id(Id::Sym::LazyMore));
        }
        break;
      }
      case Char::kQuest : {
        if (greedy) {
          push_operator(Id(Id::Sym::Quest));
        } else {
          push_operator(Id(Id::Sym::LazyQuest));
        }
        break;
      }
      case Char::kBackslash: {
        // attention to order of precedence for regex operators
        ++it;
        FALL_THROUGH;
      }
      default: {
        vector.emplace_back(*it);
        break;
      }
    }
    switch (op) {
      case Char::kEither: {
        concat_stack.top() = false;
        break;
      }
      case Char::kParen: {
        concat_stack.push(false);
        break;
      }
      case Char::kParenEnd: {
        concat_stack.pop();
        if (concat_stack.top()) {
          push_operator(Id(Id::Sym::Concat));
        }
        concat_stack.top() = true;
        break;
      }
      case Char::kMore:
      case Char::kQuest: {
        break;
      }
      case Char::kAny:
      case Char::kBackslash:
      default: {
        if (concat_stack.top()) {
          push_operator(Id(Id::Sym::Concat));
        }
        concat_stack.top() = true;
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
