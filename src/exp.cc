//
// Copyright [2020] <inhzus>
//

#include "regex/exp.h"

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
//    case More:
//    case PosMore:
//    case RelMore:
//    case Quest:
//    case PosQuest:
//    case RelQuest:
//    case Repeat: return 5;
    case Concat:return 6;
    case Either:return 8;
    default:return 0;
  }
}

#define FALL_THROUGH do {} while (0)
Exp Exp::FromStr(const std::string &s) {
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
      case Id::Sym::PosMore:
      case Id::Sym::RelMore:
      case Id::Sym::Quest:
      case Id::Sym::PosQuest:
      case Id::Sym::RelQuest:
      case Id::Sym::Repeat:
      case Id::Sym::PosRepeat:
      case Id::Sym::RelRepeat: vector.push_back(id);
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
  enum Quantifier { Greedy, Possessive, Reluctant };
  auto get_quantifier = [&s = s, &it = it]() -> Quantifier {
    auto next(it + 1);
    Quantifier q = Greedy;
    if (next == s.end()) return q;
    if (*next == Char::kPlus) {
      q = Possessive;
    } else if (*next == Char::kQuest) {
      q = Reluctant;
    } else {
      return q;
    }
    it = next;
    return q;
  };
  for (; s.end() != it; ++it) {
    char op = *it;
    switch (op) {
      case Char::kAny: {
        vector.emplace_back(Id::Sym::Any);
        break;
      }
      case Char::kBrace: {
        size_t lower_bound = 0, upper_bound = 0;
        for (++it; *it != Char::kBraceSplit; ++it) {
          lower_bound = lower_bound * 10 + *it - '0';
        }
        ++it;
        if (*it == Char::kBraceEnd) {
          upper_bound = std::numeric_limits<size_t>::max();
        } else {
          for (; *it != Char::kBraceEnd; ++it) {
            upper_bound = upper_bound * 10 + *it - '0';
          }
        }
        switch (get_quantifier()) {
          case Greedy:
            push_operator(
                Id::RepeatId(Id::Sym::Repeat, lower_bound, upper_bound));
            break;
          case Possessive:
            push_operator(
                Id::RepeatId(Id::Sym::PosRepeat, lower_bound, upper_bound));
            break;
          case Reluctant:
            push_operator(
                Id::RepeatId(Id::Sym::RelRepeat, lower_bound, upper_bound));
            break;
        }
        break;
      }
      case Char::kEither: {
        push_operator(Id(Id::Sym::Either));
        break;
      }
      case Char::kParen: {
        auto quest(it + 1);
        if (quest == s.end() || *quest != Char::kParenFLag) {
          stack.push(Id::ParenId(store_idx++));
          break;
        }
        auto flag(quest + 1);
        if (flag == s.end()) break;  // grammar error
        switch (*flag) {
          case Char::kAheadFlag: {
            stack.push(Id(Id::Sym::AheadPr));
            break;
          }
          case Char::kNegAheadFlag: {
            stack.push(Id(Id::Sym::NegAheadPr));
            break;
          }
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
        switch (get_quantifier()) {
          case Greedy:push_operator(Id(Id::Sym::More));
            break;
          case Possessive: push_operator(Id(Id::Sym::PosMore));
            break;
          case Reluctant: push_operator(Id(Id::Sym::RelMore));
            break;
        }
        break;
      }
      case Char::kQuest: {
        switch (get_quantifier()) {
          case Greedy: push_operator(Id(Id::Sym::Quest));
            break;
          case Possessive: push_operator(Id(Id::Sym::PosQuest));
            break;
          case Reluctant: push_operator(Id(Id::Sym::RelQuest));
            break;
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
//        bool group_ignored = stack.top().sym.IsIgnored();
//        if (concat_stack.top() && !group_ignored) {
        if (concat_stack.top()) {
          push_operator(Id(Id::Sym::Concat));
        }
//        if (!group_ignored)
        concat_stack.top() = true;
        break;
      }
      case Char::kMore:
      case Char::kQuest:
      case Char::kBrace: {
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
  return {std::move(vector), store_idx};
}

}  // namespace regex
