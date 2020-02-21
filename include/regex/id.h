//
// Copyright [2020] <inhzus>
//
#ifndef REGEX_ID_H_
#define REGEX_ID_H_

#include <stack>
#include <string>
#include <utility>
#include <vector>

namespace regex {

struct Char {
  static const char kAny = '.', kBackslash = '\\', kConcat = '.', kEither = '|',
      kMore = '*', kParen = '(', kParenEnd = ')', kPlus = '+',
      kQuest = '?';
};

struct Id {
 public:
  struct Sym {
   public:
    enum _Inner {
      Any,  // "."
      Char,  // a character
      Concat,  // concatenate characters
      Either,  // "|"
      More, LazyMore,  // "*", "*?"
      Paren, ParenEnd,  // "(", ")"
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
    [[nodiscard]] size_t order() const { return order_; }

   private:
    static size_t Order(_Inner inner);

    _Inner inner_;
    size_t order_;
  };

  explicit Id(Sym::_Inner sym) : sym(sym), ch() {}
  explicit Id(char ch) : sym(Sym::Char), ch(ch) {}

  Sym sym;
  char ch;
};

std::vector<Id> StrToPostfixIds(const std::string &s);

}  // namespace regex

#endif  // REGEX_ID_H_
