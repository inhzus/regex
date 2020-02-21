//
// Copyright [2020] <inhzus>
//

#include "regex/exp.h"

#include <catch2/catch.hpp>

using regex::Id;

std::string ExpToString(const std::vector<Id> &vector) {
  std::string s;
  for (const auto &id : vector) {
    switch (static_cast<int>(id.sym)) {
      case Id::Sym::Any: s.push_back('_');
        break;
      case Id::Sym::Char: s.push_back(id.ch);
        break;
      case Id::Sym::Concat: s.push_back('.');
        break;
      case Id::Sym::Either: s.push_back('|');
        break;
      case Id::Sym::More: s.push_back('*');
        break;
      case Id::Sym::LazyMore: s += "*?";
        break;
      case Id::Sym::Paren: s.push_back('(');
        break;
      case Id::Sym::ParenEnd: s.push_back(')');
        break;
      case Id::Sym::Quest: s.push_back('?');
        break;
      case Id::Sym::LazyQuest: s += "??";
        break;
      default:assert(false);
        break;
    }
  }
  return s;
}

std::string InfixToPostfix(const std::string &s) {
  return ExpToString(regex::Exp(s).Post());
}

TEST_CASE("exp", "[exp]") {
  REQUIRE("aa." == InfixToPostfix("aa"));
  REQUIRE("aa|" == InfixToPostfix("a|a"));
  REQUIRE("aa|.." == InfixToPostfix("aa\\|"));

  REQUIRE("ab.(c." == InfixToPostfix("(ab)c"));
  REQUIRE("abc.(d.." == InfixToPostfix("a(bc)d"));
  REQUIRE("a*bc|(." == InfixToPostfix("a*(b|c)"));
  REQUIRE("a*bcd*efg.|(..(?h..|i|" == InfixToPostfix("a*|b(cd*(e|fg))?h|i"));
}
