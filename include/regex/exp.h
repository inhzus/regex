//
// Copyright [2020] <inhzus>
//
#ifndef REGEX_EXP_H_
#define REGEX_EXP_H_

#include <stack>
#include <string>
#include <utility>
#include <vector>
#include <unordered_map>

namespace regex {

namespace ch {
static const char kAheadFlag = '=', kNegAheadFlag = '!', kAny = '.',
    kBackslash = '\\', kBrace = '{', kBraceEnd = '}', kBraceSplit = ',',
    kConcat = '.', kEither = '|', kMore = '*', kNamedFlag = 'P',
    kNEqualFlag = '=', kNLeftFlag = '<', kNRightFlag = '>', kParen = '(',
    kParenEnd = ')', kParenFLag = '?', kPlus = '+', kQuest = '?',
    kUnParenFlag = ':';
};

struct Id {
 public:
  struct Sym {
   public:
    enum _Inner {
      AheadPr, NegAheadPr,  // "(?=", "(?!"
      Any,  // "."
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
      Repeat, PosRepeat, RelRepeat  // "{m,n}", "{m,n}+", "{m,n}?"
    };
    explicit Sym(Sym::_Inner inner) : inner_(inner), order_(Order(inner)) {}
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
      return inner_ == Paren || inner_ == UnParen || inner_ == NamedPr
          || inner_ == AheadPr || inner_ == NegAheadPr || inner_ == RefPr;
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

  explicit Id(Sym::_Inner sym) : sym(sym), ch() {}
  explicit Id(char ch) : sym(Sym::Char), ch(ch) {}

  Sym sym;
  union {
    char ch;
    struct {
      size_t idx;
    } named, ref, store;
    struct {
      size_t lower;
      size_t upper;
    } repeat;
  };

 private:
  Id(Sym::_Inner sym, size_t idx) : sym(sym), store({idx}) {}
  Id(Sym::_Inner sym, size_t lower, size_t upper) :
      sym(sym), repeat({lower, upper}) {}
};

struct Exp {
  static Exp FromStr(const std::string &s);

  size_t group_num;
  std::vector<Id> ids;
  std::unordered_map<std::string, size_t> named_group;
};

}  // namespace regex

#endif  // REGEX_EXP_H_
