//
// Copyright [2020] <inhzus>
//

#include "regex/graph.h"

#include <catch2/catch.hpp>
#include <string>
#include <vector>

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
  auto exp = regex::Exp::FromStr(infix);
  REQUIRE(IdsToStr(exp.ids) == postfix);
  return regex::Graph::Compile(std::move(exp));
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

TEST_CASE("graph match any and backslash", "[graph]") {
  auto graph = CompileInfix(".*a", "_*a.");
  REQUIRE(15 == graph.Match("abcdefghijklmna"));
  graph = CompileInfix("a.?b", "a_?b..");
  REQUIRE(3 == graph.Match("acb"));
  REQUIRE(3 == graph.Match("abb"));
  graph = CompileInfix("a.??b", "a_??b..");
  REQUIRE(2 == graph.Match("abb"));

  graph = CompileInfix(R"(\(\.?\))", "(.?)..");
  REQUIRE(3 == graph.Match("(.)"));
  REQUIRE(2 == graph.Match("()"));
}

TEST_CASE("graph match function that supports break backtrack", "[graph]") {
  auto graph = CompileInfix("ab", "ab.");
  REQUIRE(2 == graph.Match("ab"));

  graph = CompileInfix("aa??", "aa??.");
  REQUIRE(1 == graph.Match("aa"));

  graph = CompileInfix("aa*|b(cd*(e|fg))?h|i", "aa*.bcd*efg.|(..(?h..|i|");
  REQUIRE(8 == graph.Match("bcdddfgh"));
  REQUIRE(1 == graph.Match("i"));
  REQUIRE(2 == graph.Match("bh"));
}

TEST_CASE("graph match groups", "[graph]") {
  auto graph = CompileInfix("aa*|b(cd*(e|fg))?h|i", "aa*.bcd*efg.|(..(?h..|i|");

  std::vector<std::string> groups;
  REQUIRE(graph.Match("bcdddfgh", &groups));
  REQUIRE(3 == groups.size());
  REQUIRE("bcdddfgh" == groups[0]);
  REQUIRE("cdddfg" == groups[1]);
  REQUIRE("fg" == groups[2]);
}

TEST_CASE("graph match non-captured groups", "[graph]") {
  auto graph = CompileInfix(
      "aa*|b(?:cd*(?:e|fg))?h|i", "aa*.bcd*efg.|..?h..|i|");

  REQUIRE(8 == graph.Match("bcdddfgh"));
  REQUIRE(1 == graph.Match("i"));
  REQUIRE(2 == graph.Match("bh"));
  std::vector<std::string> groups;
  REQUIRE(graph.Match("bcdddfgh", &groups));
  REQUIRE(1 == groups.size());
  REQUIRE("bcdddfgh" == groups[0]);
}

TEST_CASE("graph look-ahead", "[graph]") {
  auto graph = CompileInfix("a(?=b)(b|c)", "ab(=bc|(..");

  std::vector<std::string> groups;
  REQUIRE(graph.Match("ab", &groups));
  REQUIRE("ab" == groups[0]);
  REQUIRE("b" == groups[1]);

  graph = CompileInfix("a(?=(b))(b|c)", "ab((=bc|(..");
  REQUIRE(graph.Match("ab", &groups));
  REQUIRE("ab" == groups[0]);
  REQUIRE("b" == groups[1]);
  REQUIRE("b" == groups[2]);

  REQUIRE_FALSE(graph.Match("ac", &groups));

  graph = CompileInfix("a(?!(b))(b|c)", "ab((!bc|(..");
  REQUIRE(graph.Match("ac", &groups));
  REQUIRE("ac" == groups[0]);
  REQUIRE("b" == groups[1]);
  REQUIRE("c" == groups[2]);

  REQUIRE_FALSE(graph.Match("ab", &groups));
}

TEST_CASE("graph possessive quantifiers", "[graph]") {
  auto graph = CompileInfix(".*+b", "_*+b.");
//  graph.DrawMermaid();
  REQUIRE_FALSE(graph.Match("b", nullptr));
  graph = CompileInfix("a*+b", "a*+b.");
  REQUIRE(4 == graph.Match("aaab"));
  REQUIRE(4 == graph.Match("aaab"));
  graph = CompileInfix(".*b", "_*b.");
  REQUIRE(graph.Match("aaab", nullptr));

  graph = CompileInfix("a?+a", "a?+a.");
//  graph.DrawMermaid();
  REQUIRE_FALSE(graph.Match("a", nullptr));
  REQUIRE(graph.Match("aa", nullptr));
}

TEST_CASE("graph {m,n}", "[graph]") {
  auto graph = CompileInfix("a{1,5}b", "a{1,5}b.");
  REQUIRE(2 == graph.Match("ab"));
  REQUIRE(6 == graph.Match("aaaaab"));
  REQUIRE(-1 == graph.Match("b"));
  REQUIRE(-1 == graph.Match("aaaaaab"));

  graph = CompileInfix("a{,1}b", "a{,1}b.");
  REQUIRE(1 == graph.Match("b"));
  REQUIRE(2 == graph.Match("ab"));
  REQUIRE(-1 == graph.Match("aab"));

  graph = CompileInfix("a{1,}b", "a{1,}b.");
  REQUIRE(-1 == graph.Match("b"));
  REQUIRE(2 == graph.Match("ab"));

  graph = CompileInfix("a{,1}+a", "a{,1}+a.");
  REQUIRE(-1 == graph.Match("a"));

  graph = CompileInfix("a{,1}?a", "a{,1}?a.");
  REQUIRE(1 == graph.Match("aa"));

  graph = CompileInfix("a{2}", "a{2,2}");
  REQUIRE(-1 == graph.Match("a"));
  REQUIRE(2 == graph.Match("aa"));
  REQUIRE(2 == graph.Match("aaa"));
}

TEST_CASE("graph back referencing", "[graph]") {
//  auto graph = CompileInfix("a(?P<b>b)(?P<c>b|c)", "ab(<b>bc|(<c>..");
//  std::vector<std::string> groups;
//  REQUIRE(graph.Match("abc", &groups));
//  REQUIRE("b" == groups[1]);
//  REQUIRE("c" == groups[2]);

  auto graph = CompileInfix("(?P<a>b|c)(?P=a)d", "bc|(<a><1>d..");
//  graph.DrawMermaid();
  REQUIRE(-1 == graph.Match("bcd"));
  REQUIRE(3 == graph.Match("bbd"));
  REQUIRE(3 == graph.Match("ccd"));
}
