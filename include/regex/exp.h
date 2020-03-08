//
// Copyright [2020] <inhzus>
//
#ifndef REGEX_EXP_H_
#define REGEX_EXP_H_

#include <algorithm>
#include <cassert>
#include <stack>
#include <string>
#include <utility>
#include <vector>
#include <unordered_map>

namespace regex {

namespace ch {
static constexpr const char kAheadFlag = '=', kNegAheadFlag = '!', kAny = '.',
    kAtomicFlag = '>', kBackslash = '\\', kBrace = '{', kBraceEnd = '}',
    kBraceSplit = ',', kBrk = '[', kBrkEnd = ']', kBrkRange = '-',
    kBrkReverse = '^', kConcat = '.', kEither = '|', kMore = '*',
    kNamedFlag = 'P', kNEqualFlag = '=', kNLeftFlag = '<', kNRightFlag = '>',
    kParen = '(', kParenEnd = ')', kParenFLag = '?', kPlus = '+', kQuest = '?',
    kUnParenFlag = ':';
};

// character classes
struct CharSet {
  struct Range {
    explicit Range(char val) : val(val), last(val) {}
    Range(char val, char last) : val(val), last(last) {}
    bool operator<(const Range &range) const {
      return val < range.val || (val == range.val && last < range.last);
    }

    char val;
    char last;  // `last` contained
  };

  struct Group {
    Group() : ranges() {}
    inline Group &Insert(char ch) {
      return Insert(ch, ch);
    }
    inline Group &Insert(char val, char last) {
      ranges.emplace_back(val, last);
      return *this;
    }
    inline Group &MoveAppend(Group *group) {
      std::move(group->ranges.begin(), group->ranges.end(),
                std::back_inserter(ranges));
      group->ranges.clear();
      return *this;
    }
    void Fold() {
      std::sort(ranges.begin(), ranges.end());
      std::vector<Range> result;
      auto it = ranges.begin(), cur = it++;
      while (it != ranges.end()) {
        if (cur->last + 1 >= it->val) {
          cur->last = std::max(cur->last, it->last);
        } else {
          result.push_back(*cur);
          cur = it;
        }
        ++it;
      }
      result.push_back(*cur);
      std::swap(ranges, result);
    }
    [[nodiscard]] bool Contains(char ch) const {
      for (const auto &range : ranges) {
        if (ch >= range.val && ch <= range.last) {
          return true;
        }
      }
      return false;
    }

    std::vector<Range> ranges;
  };

  CharSet() : pos(), negs() {}
  inline void Fold() {
    pos.Fold();
  }
  [[nodiscard]] bool Contains(char ch) const {
    if (pos.Contains(ch)) return true;
    for (const auto &group : negs) {
      if (!group.Contains(ch)) {
        return true;
      }
    }
    return false;
  }

  Group pos;
  std::vector<Group> negs;
};

struct Id {
 public:
  struct Sym {
   public:
    enum _Inner {
      AheadPr, NegAheadPr,  // "(?=", "(?!"
      Any,  // "."
      AtomicPr,  // "(?>...)"
      Char,  // a character
      Concat,  // concatenate characters
      Either,  // "|"
      More, RelMore, PosMore,  // "*", "*+", "*?"
      NamedPr,  // "(?P<name>...)"
      Paren, ParenEnd,  // "(", ")"
      UnParen,  // "(?:"
      Plus, PosPlus, RelPlus,  // "+", "++", "+?"
      Quest, PosQuest, RelQuest,  // "?", "?+", "??"
      RefPr,  // "(?P=name)"
      Repeat, PosRepeat, RelRepeat,  // "{m,n}", "{m,n}+", "{m,n}?"
      Set, SetEx,  // "[...]", "[^...]"
    };
    explicit Sym(Sym::_Inner inner) : inner_(inner), order_(Order(inner)) {}
    Sym &operator=(Sym::_Inner inner) {
      inner_ = inner;
      order_ = Order(inner);
      return *this;
    }
    bool operator==(Sym::_Inner inner) { return inner == inner_; }
    bool operator!=(Sym::_Inner inner) { return inner != inner_; }
    explicit operator int() const {
      return static_cast<int>(inner_);
    }

    [[nodiscard]] bool IsOperand() const {
      return inner_ == Any || inner_ == Char;
    }
    [[nodiscard]] bool IsOperator() const { return !IsOperand(); }
    [[nodiscard]] bool IsParen() const {
      return inner_ == Paren || inner_ == UnParen || inner_ == AheadPr ||
          inner_ == NegAheadPr || inner_ == AtomicPr || inner_ == NamedPr ||
          inner_ == RefPr;
    }
    [[nodiscard]] size_t order() const { return order_; }

   private:
    static size_t Order(_Inner inner);

    _Inner inner_;
    size_t order_;
  };

  inline static Id NamedId(size_t idx) {
    return Id(Id::Sym::NamedPr, idx);
  }
  inline static Id ParenId(size_t idx) {
    return Id(Id::Sym::Paren, idx);
  }
  inline static Id RepeatId(Sym::_Inner sym, size_t lower, size_t upper) {
    return Id(sym, lower, upper);
  }
  inline static Id RefId(size_t idx) {
    return Id(Id::Sym::RefPr, idx);
  }
  inline static Id SetId() {
    Id id(Sym::Set);
    id.set = new std::remove_reference_t<decltype(*id.set)>{CharSet()};
    return id;
  }

  explicit Id(Sym::_Inner sym) : sym(sym), ch() {}
  explicit Id(char ch) : sym(Sym::Char), ch(ch) {}
  Id(const Id &id) : sym(id.sym) {
    switch (static_cast<int>(sym)) {
      case Sym::Repeat:
      case Sym::RelRepeat:
      case Sym::PosRepeat:
        repeat = new std::remove_reference_t<decltype(*repeat)>(
            {id.repeat->lower, id.repeat->upper});
        break;
      case Sym::Set:
        set = new std::remove_reference_t<
            decltype(*set)>({id.set->val});
        break;
      default:store = id.store;
        break;
    }
  }
  Id(Id &&id) : sym(id.sym) {
    set = id.set;
    id.set = {};
    id.sym = Sym(Sym::Char);
  }
  ~Id() {
    switch (static_cast<int>(sym)) {
      case Sym::Repeat:
      case Sym::RelRepeat:
      case Sym::PosRepeat: delete repeat;
        break;
      case Sym::Set:
      case Sym::SetEx: delete set;
        break;
      default:break;
    }
  }

  Sym sym;
  union {
    char ch;
    struct {
      size_t idx;
    } named, ref, store;
    struct {
      size_t lower;
      size_t upper;
    } *repeat;
    struct {
      CharSet val;
    } *set;
  };

 private:
  Id(Sym::_Inner sym, size_t idx) : sym(sym), store({idx}) {}
  Id(Sym::_Inner sym, size_t lower, size_t upper) : sym(sym) {
    repeat = new std::remove_reference_t<decltype(*repeat)>({lower, upper});
  }
};

struct Exp {
  static Exp FromStr(const std::string &s);

  size_t group_num;
  std::vector<Id> ids;
  std::unordered_map<std::string, size_t> named_group;
};

}  // namespace regex

#endif  // REGEX_EXP_H_
