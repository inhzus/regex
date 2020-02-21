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
  static const char kAny = '.', kBackslash = '\\', kConcat = '.', kEither = '|',
      kMore = '*', kLeftParen = '(', kRightParen = ')', kPlus = '+',
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
    explicit Sym(Sym::_Inner inner) : inner_(inner) {}
    bool operator==(Sym::_Inner inner) { return inner == inner_; }
    bool operator!=(Sym::_Inner inner) { return inner != inner_; }
    explicit operator int() const {
      return static_cast<int>(inner_);
    }

    [[nodiscard]] bool IsOperand() const {
      return inner_ == Any || inner_ == Char;
    }
    [[nodiscard]] bool IsOperator() const {
      return !IsOperand();
    }
    [[nodiscard]] size_t Order() const;

   private:
    _Inner inner_;
  } sym;
  char ch;

  explicit Id(Sym::_Inner sym) : sym(sym), ch() {}
  explicit Id(char ch) : sym(Sym::Char), ch(ch) {}
};

class Exp {
 public:
  explicit Exp(const std::string &s) :  // NOLINT
      string_(s) {}
  explicit Exp(std::string &&s) :
      string_(s) {}
  Exp(const Exp &) = delete;
  Exp operator=(const Exp &) = delete;

  [[nodiscard]] std::vector<Id> Post() const;

 private:
  const std::string string_;
};

}  // namespace regex

#endif  // REGEX_EXP_H_
