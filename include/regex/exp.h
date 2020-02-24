//
// Copyright [2020] <inhzus>
//
#ifndef REGEX_EXP_H_
#define REGEX_EXP_H_

#include <stack>
#include <string>
#include <utility>
#include <vector>

namespace regex {

struct Char {
  static const char kAheadFlag = '=', kNegAheadFlag = '!', kAny = '.',
      kBackslash = '\\', kConcat = '.', kEither = '|', kMore = '*',
      kParen = '(', kParenEnd = ')', kParenFLag = '?', kPlus = '+',
      kQuest = '?', kUnParenFlag = ':';
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
      More, LazyMore,  // "*", "*?"
      Paren, ParenEnd,  // "(", ")"
      UnParen,  // "(?:"
      Plus, LazyPlus,  // "+", "+?"
      Quest, LazyQuest  // "?", "??"
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
      return inner_ == Paren || inner_ == UnParen
          || inner_ == AheadPr || inner_ == NegAheadPr;
    }
    [[nodiscard]] bool IsIgnored() const {
      return inner_ == AheadPr || inner_ == NegAheadPr;
    }
    [[nodiscard]] size_t order() const { return order_; }

   private:
    static size_t Order(_Inner inner);

    _Inner inner_;
    size_t order_;
  };

  explicit Id(Sym::_Inner sym) : sym(sym), ch() {}
  explicit Id(char ch) : sym(Sym::Char), ch(ch) {}

  Sym sym;
  union {
    char ch;
    struct {
      size_t idx;
    } store;
  };
};

struct Exp {
  static Exp FromStr(const std::string &s);

  std::vector<Id> ids;
  size_t group_num;
};

}  // namespace regex

#endif  // REGEX_EXP_H_
