//
// Copyright [2020] <inhzus>
//

#include "regex/graph.h"

#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>

#include "test/utils.h"

TEST_CASE("graph match concat", "[graph]") {
  auto graph = regex::Graph::CompilePostfix("aa.");
  REQUIRE(2 == graph.Match("aa"));
  REQUIRE(2 == graph.Match("aaa"));
  REQUIRE(-1 == graph.Match("a"));
  REQUIRE(-1 == graph.Match("b"));
  REQUIRE(-1 == graph.Match(""));
}

TEST_CASE("graph match 0 or more", "[graph]") {
  auto graph = regex::Graph::CompilePostfix("aa*.");
  REQUIRE(-1 == graph.Match(""));
  REQUIRE(-1 == graph.Match("b"));
  REQUIRE(1 == graph.Match("a"));
  REQUIRE(2 == graph.Match("aa"));
  REQUIRE(2 == graph.Match("aab"));

  graph = regex::Graph::CompilePostfix("ab|*a.");
  REQUIRE(4 == graph.Match("babac"));
}

TEST_CASE("graph match either one", "[graph]") {
  auto graph = regex::Graph::CompilePostfix("ba*|");
  REQUIRE(1 == graph.Match("a"));
  REQUIRE(1 == graph.Match("b"));
}

TEST_CASE("graph match the element or not", "[graph]") {
  auto graph = regex::Graph::CompilePostfix("a?b.");
  REQUIRE(2 == graph.Match("ab"));
  REQUIRE(1 == graph.Match("b"));
  REQUIRE(-1 == graph.Match("a"));
}

TEST_CASE("graph greedy or lazy", "[graph]") {
  auto graph = regex::Graph::CompilePostfix("aa*?.");
  REQUIRE(1 == graph.Match("a"));
  REQUIRE(1 == graph.Match("aa"));

  graph = regex::Graph::CompilePostfix("ab|*?a.");
  REQUIRE(2 == graph.Match("babac"));

  graph = regex::Graph::CompilePostfix("aa??.");
  REQUIRE(1 == graph.Match("aa"));
}

inline regex::Graph
CompileInfix(const std::string &infix, const std::string &postfix) {
  auto ids = regex::StrToPostfixIds(infix);
  REQUIRE(IdsToStr(ids) == postfix);
  return regex::Graph::Compile(std::move(ids));
}

TEST_CASE("graph of infix order", "[graph]") {
  auto graph = CompileInfix("aa", "aa.");
  REQUIRE(2 == graph.Match("aa"));
  REQUIRE(2 == graph.Match("aaa"));
  REQUIRE(-1 == graph.Match("a"));
  REQUIRE(-1 == graph.Match("b"));
  REQUIRE(-1 == graph.Match(""));

  graph = CompileInfix("aa*", "aa*.");
  REQUIRE(-1 == graph.Match(""));
  REQUIRE(-1 == graph.Match("b"));
  REQUIRE(1 == graph.Match("a"));
  REQUIRE(2 == graph.Match("aa"));
  REQUIRE(2 == graph.Match("aab"));

  graph = CompileInfix("(a|b)*a", "ab|(*a.");
  REQUIRE(4 == graph.Match("babac"));

  graph = CompileInfix("b|a*", "ba*|");
  REQUIRE(1 == graph.Match("a"));
  REQUIRE(1 == graph.Match("b"));

  graph = CompileInfix("a?b", "a?b.");
  REQUIRE(2 == graph.Match("ab"));
  REQUIRE(1 == graph.Match("b"));
  REQUIRE(-1 == graph.Match("a"));

  graph = CompileInfix("aa*?", "aa*?.");
  REQUIRE(1 == graph.Match("a"));
  REQUIRE(1 == graph.Match("aa"));

  graph = CompileInfix("(a|b)*?a", "ab|(*?a.");
  REQUIRE(2 == graph.Match("babac"));

  graph = CompileInfix("aa??", "aa??.");
  REQUIRE(1 == graph.Match("aa"));

  graph = CompileInfix("aa*|b(cd*(e|fg))?h|i", "aa*.bcd*efg.|(..(?h..|i|");
  REQUIRE(8 == graph.Match("bcdddfgh"));
  REQUIRE(1 == graph.Match("i"));
  REQUIRE(2 == graph.Match("bh"));
}
