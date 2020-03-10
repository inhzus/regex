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

namespace es {
// escape characters
static const char kNum = 'd', kNumEx = 'D', kWord = 'w', kWordEx = 'W',
    kWSpace = 's', kWSpaceEx = 'S';
static const char
    *kSetEscaped = "-]",
    *kEscaped = "\\^$.|?*+()[{";
}  // namespace es

enum class RangeT : char {
  Char, Exclude, Include
};

static std::tuple<RangeT, char, CharSet::Group>
ParseBackSlash(char ch, std::string_view escaped) {
  RangeT r;
  CharSet::Group group;
  char val;
  switch (ch) {
    case es::kNum:
    case es::kWord:
    case es::kWSpace: r = RangeT::Include;
      break;
    case es::kNumEx:
    case es::kWordEx:
    case es::kWSpaceEx:r = RangeT::Exclude;
      break;
    default: r = RangeT::Char;
      break;
  }
  switch (ch) {
    case es::kNum:
    case es::kNumEx: {
      group.Insert('0', '9');
      break;
    }
    case es::kWord:
    case es::kWordEx: {
      group.Insert('0', '9').Insert('a', 'z')
          .Insert('A', 'Z').Insert('_');
      break;
    }
    case es::kWSpace:
    case es::kWSpaceEx: {
      group.Insert('\t', '\n')  // "\t\n"
          .Insert('\f', '\r')  // "\f\r"
          .Insert(' ');
      break;
    }
    default: {
      for (auto p : escaped) {
        if (ch == p) {
          val = ch;
        }
      }
      break;
    }
  }
  return std::make_tuple(r, val, std::move(group));
}

#define FALL_THROUGH do {} while (0)
Exp Exp::FromStr(const std::string &s) {
  std::vector<Id> vector;
  std::string::const_iterator it(s.begin());
  std::stack<Id, std::vector<Id>> stack;
  std::stack<bool, std::vector<bool>> concat_stack;
  std::unordered_map<std::string, size_t> named;
  concat_stack.push(false);
  size_t store_idx = 1;
  auto push_operator = [&vector = vector, &stack = stack](Id &&id) {
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
      case Id::Sym::RelRepeat:
      case Id::Sym::Set: vector.push_back(std::move(id));
        return;
    }
    while (!stack.empty()) {
      Id &top = stack.top();
      if (top.sym.IsParen()) break;
      if (top.sym.order() <= id.sym.order()) {
        stack.pop();
        vector.push_back(std::move(top));
      } else {
        break;
      }
    }
    stack.push(std::move(id));
  };
  enum Quantifier { Greedy, Possessive, Reluctant };
  auto get_quantifier = [&s = s, &it = it]() -> Quantifier {
    auto next(it + 1);
    Quantifier q = Greedy;
    if (next == s.end()) return q;
    if (*next == ch::kPlus) {
      q = Possessive;
    } else if (*next == ch::kQuest) {
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
      case ch::kAny: {
        vector.emplace_back(Id::Sym::Any);
        break;
      }
      case ch::kBegin: {
        vector.emplace_back(Id::Sym::Begin);
        break;
      }
      case ch::kBrace: {
        size_t lower_bound = 0, upper_bound = 0;
        for (++it; *it != ch::kBraceSplit && *it != ch::kBraceEnd; ++it) {
          lower_bound = lower_bound * 10 + *it - '0';
        }
        if (*it == ch::kBraceEnd) {
          upper_bound = lower_bound;
        } else if (++it; *it == ch::kBraceEnd) {
          upper_bound = std::numeric_limits<size_t>::max();
        } else {
          for (; *it != ch::kBraceEnd; ++it) {
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
      case ch::kBrk: {
        Id id(Id::SetId());
        CharSet &set = id.set->val;
        ++it;
        if (*it == ch::kBrkReverse) {
          id.sym = Id::Sym(Id::Sym::SetEx);
          ++it;
        }
        if (*it == ch::kBrkRange || *it == ch::kBrkEnd) {
          set.pos.Insert(*it);
          ++it;
        }
        for (; it != s.end() && *it != ch::kBrkEnd; ++it) {
          if (*it == ch::kBackslash) {
            ++it;
            auto[r, val, group] = ParseBackSlash(*it, es::kSetEscaped);
            switch (r) {
              case RangeT::Char:set.pos.Insert(val);
                break;
              case RangeT::Include: set.pos.MoveAppend(&group);
                break;
              case RangeT::Exclude: set.negs.push_back(std::move(group));
                break;
            }
          } else if (*it != ch::kBrkRange) {
            set.pos.Insert(*it);
          } else if (char nch = *(it + 1); nch == ch::kBrkEnd) {
            set.pos.Insert(ch::kBrkRange);
          } else {
            char pch = *(it - 1);
            assert(pch <= nch);
            set.pos.Insert(pch, nch);
            ++it;
          }
        }
        set.Fold();
        vector.push_back(std::move(id));
        break;
      }
      case ch::kEither: {
        push_operator(Id(Id::Sym::Either));
        break;
      }
      case ch::kEnd: {
        vector.emplace_back(Id::Sym::End);
        break;
      }
      case ch::kParen: {
        auto quest(it + 1);
        if (quest == s.end() || *quest != ch::kParenFLag) {
          stack.push(Id::ParenId(store_idx++));
          break;
        }
        auto flag(quest + 1);
        if (flag == s.end()) break;  // grammar error
        switch (*flag) {
          case ch::kAheadFlag: {
            stack.push(Id(Id::Sym::AheadPr));
            break;
          }
          case ch::kNegAheadFlag: {
            stack.push(Id(Id::Sym::NegAheadPr));
            break;
          }
          case ch::kAtomicFlag: {
            stack.push(Id(Id::Sym::AtomicPr));
            break;
          }
          case ch::kNamedFlag: {
            ++flag;  // *flag == '<' or '='
            if (*flag == ch::kNLeftFlag) {
              std::string::const_iterator left = ++flag;
              for (; *flag != ch::kNRightFlag; ++flag) {}
              named[std::string(left, flag)] = store_idx;
              stack.push(Id::NamedId(store_idx++));
            } else if (*flag == ch::kNEqualFlag) {
              std::string::const_iterator left = ++flag;
              for (; *flag != ch::kParenEnd; ++flag) {}
              auto find = named.find(std::string(left, flag));
              assert(find != named.end());
              stack.push(Id::RefId(find->second));
              --flag;  // step back to the character before ')'
            } else {
              assert(false);
            }
            break;
          }
          case ch::kUnParenFlag: {
            stack.push(Id(Id::Sym::UnParen));
            break;
          }
          default: break;
        }
        it = flag;
        break;
      }
      case ch::kParenEnd: {
        while (true) {
          assert(!stack.empty());
          Id &id = stack.top();
          stack.pop();
          bool to_break = id.sym.IsParen();
          vector.push_back(std::move(id));
          if (to_break) {
            break;
          }
        }
        break;
      }
      case ch::kMore: {
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
      case ch::kQuest: {
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
      case ch::kBackslash: {
        // attention to order of precedence for regex operators
        ++it;
        auto[r, val, group] = ParseBackSlash(*it, es::kEscaped);
        if (r == RangeT::Char) {
          vector.emplace_back(val);
        } else {
          Id id(Id::SetId());
          if (r == RangeT::Exclude) id.sym = Id::Sym::SetEx;
          id.set->val.pos.MoveAppend(&group);
        }
        break;
      }
      default: {
        vector.emplace_back(*it);
        break;
      }
    }
    switch (op) {
      case ch::kEither: {
        concat_stack.top() = false;
        break;
      }
      case ch::kParen: {
        concat_stack.push(false);
        break;
      }
      case ch::kParenEnd: {
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
      case ch::kMore:
      case ch::kQuest:
      case ch::kBrace: {
        break;
      }
      case ch::kAny:
      case ch::kBackslash:
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
    vector.push_back(std::move(stack.top()));
    stack.pop();
  }
  return {store_idx, std::move(vector), std::move(named)};
}

}  // namespace regex
