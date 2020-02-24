//
// Copyright [2020] <inhzus>
//

#include "regex/exp.h"

#include <catch2/catch.hpp>

#include "test/utils.h"

std::string InfixToPostfix(const std::string &s) {
  return IdsToStr(regex::Exp::FromStr(s).ids);
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
