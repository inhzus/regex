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
  REQUIRE(2 == graph.MatchLen("aa"));
  REQUIRE(2 == graph.MatchLen("aaa"));
  REQUIRE(-1 == graph.MatchLen("a"));
  REQUIRE(-1 == graph.MatchLen("b"));
  REQUIRE(-1 == graph.MatchLen(""));
}

TEST_CASE("graph match 0 or more", "[graph]") {
  auto graph = regex::Graph::CompilePostfix("aa*.");
  REQUIRE(-1 == graph.MatchLen(""));
  REQUIRE(-1 == graph.MatchLen("b"));
  REQUIRE(1 == graph.MatchLen("a"));
  REQUIRE(2 == graph.MatchLen("aa"));
  REQUIRE(2 == graph.MatchLen("aab"));

  graph = regex::Graph::CompilePostfix("ab|*a.");
  REQUIRE(4 == graph.MatchLen("babac"));
}

TEST_CASE("graph match either one", "[graph]") {
  auto graph = regex::Graph::CompilePostfix("ba*|");
  REQUIRE(1 == graph.MatchLen("a"));
  REQUIRE(1 == graph.MatchLen("b"));
}

TEST_CASE("graph match the element or not", "[graph]") {
  auto graph = regex::Graph::CompilePostfix("a?b.");
  REQUIRE(2 == graph.MatchLen("ab"));
  REQUIRE(1 == graph.MatchLen("b"));
  REQUIRE(-1 == graph.MatchLen("a"));
}

TEST_CASE("graph greedy or lazy", "[graph]") {
  auto graph = regex::Graph::CompilePostfix("aa*?.");
  REQUIRE(1 == graph.MatchLen("a"));
  REQUIRE(1 == graph.MatchLen("aa"));

  graph = regex::Graph::CompilePostfix("ab|*?a.");
  REQUIRE(2 == graph.MatchLen("babac"));

  graph = regex::Graph::CompilePostfix("aa??.");
  REQUIRE(1 == graph.MatchLen("aa"));
}

inline regex::Graph
CompileInfix(const std::string &infix, const std::string &postfix) {
  auto exp = regex::Exp::FromStr(infix);
  REQUIRE(IdsToStr(exp.ids) == postfix);
  return regex::Graph::Compile(std::move(exp));
}

TEST_CASE("graph of infix order", "[graph]") {
  auto graph = CompileInfix("aa", "aa.");
  REQUIRE(2 == graph.MatchLen("aa"));
  REQUIRE(2 == graph.MatchLen("aaa"));
  REQUIRE(-1 == graph.MatchLen("a"));
  REQUIRE(-1 == graph.MatchLen("b"));
  REQUIRE(-1 == graph.MatchLen(""));

  graph = CompileInfix("aa*", "aa*.");
  REQUIRE(-1 == graph.MatchLen(""));
  REQUIRE(-1 == graph.MatchLen("b"));
  REQUIRE(1 == graph.MatchLen("a"));
  REQUIRE(2 == graph.MatchLen("aa"));
  REQUIRE(2 == graph.MatchLen("aab"));

  graph = CompileInfix("(a|b)*a", "ab|(*a.");
  REQUIRE(4 == graph.MatchLen("babac"));

  graph = CompileInfix("b|a*", "ba*|");
  REQUIRE(1 == graph.MatchLen("a"));
  REQUIRE(1 == graph.MatchLen("b"));

  graph = CompileInfix("a?b", "a?b.");
  REQUIRE(2 == graph.MatchLen("ab"));
  REQUIRE(1 == graph.MatchLen("b"));
  REQUIRE(-1 == graph.MatchLen("a"));

  graph = CompileInfix("aa*?", "aa*?.");
  REQUIRE(1 == graph.MatchLen("a"));
  REQUIRE(1 == graph.MatchLen("aa"));

  graph = CompileInfix("(a|b)*?a", "ab|(*?a.");
  REQUIRE(2 == graph.MatchLen("babac"));

  graph = CompileInfix("aa??", "aa??.");
  REQUIRE(1 == graph.MatchLen("aa"));

  graph = CompileInfix("aa*|b(cd*(e|fg))?h|i", "aa*.bcd*efg.|(..(?h..|i|");
  REQUIRE(8 == graph.MatchLen("bcdddfgh"));
  REQUIRE(1 == graph.MatchLen("i"));
  REQUIRE(2 == graph.MatchLen("bh"));
}

TEST_CASE("graph match any and backslash", "[graph]") {
  auto graph = CompileInfix(".*a", "_*a.");
  REQUIRE(15 == graph.MatchLen("abcdefghijklmna"));
  graph = CompileInfix("a.?b", "a_?b..");
  REQUIRE(3 == graph.MatchLen("acb"));
  REQUIRE(3 == graph.MatchLen("abb"));
  graph = CompileInfix("a.??b", "a_??b..");
  REQUIRE(2 == graph.MatchLen("abb"));

  graph = CompileInfix(R"(\(\.?\))", "(.?)..");
  REQUIRE(3 == graph.MatchLen("(.)"));
  REQUIRE(2 == graph.MatchLen("()"));
}

TEST_CASE("graph match function that supports break backtrack", "[graph]") {
  auto graph = CompileInfix("ab", "ab.");
  REQUIRE(2 == graph.MatchLen("ab"));

  graph = CompileInfix("aa??", "aa??.");
  REQUIRE(1 == graph.MatchLen("aa"));

  graph = CompileInfix("aa*|b(cd*(e|fg))?h|i", "aa*.bcd*efg.|(..(?h..|i|");
  REQUIRE(8 == graph.MatchLen("bcdddfgh"));
  REQUIRE(1 == graph.MatchLen("i"));
  REQUIRE(2 == graph.MatchLen("bh"));
}

TEST_CASE("graph match groups", "[graph]") {
  auto graph = CompileInfix("aa*|b(cd*(e|fg))?h|i", "aa*.bcd*efg.|(..(?h..|i|");

  std::vector<std::string> groups;
  REQUIRE(graph.MatchGroups("bcdddfgh", &groups));
  REQUIRE(3 == groups.size());
  REQUIRE("bcdddfgh" == groups[0]);
  REQUIRE("cdddfg" == groups[1]);
  REQUIRE("fg" == groups[2]);
}

TEST_CASE("graph match non-captured groups", "[graph]") {
  auto graph = CompileInfix(
      "aa*|b(?:cd*(?:e|fg))?h|i", "aa*.bcd*efg.|..?h..|i|");

  REQUIRE(8 == graph.MatchLen("bcdddfgh"));
  REQUIRE(1 == graph.MatchLen("i"));
  REQUIRE(2 == graph.MatchLen("bh"));
  std::vector<std::string> groups;
  REQUIRE(graph.MatchGroups("bcdddfgh", &groups));
  REQUIRE(1 == groups.size());
  REQUIRE("bcdddfgh" == groups[0]);
}

TEST_CASE("graph look-ahead", "[graph]") {
  auto graph = CompileInfix("a(?=b)(b|c)", "ab(=bc|(..");

  std::vector<std::string> groups;
  REQUIRE(graph.MatchGroups("ab", &groups));
  REQUIRE("ab" == groups[0]);
  REQUIRE("b" == groups[1]);

  graph = CompileInfix("a(?=(b))(b|c)", "ab((=bc|(..");
  REQUIRE(graph.MatchGroups("ab", &groups));
  REQUIRE("ab" == groups[0]);
  REQUIRE("b" == groups[1]);
  REQUIRE("b" == groups[2]);

  REQUIRE_FALSE(graph.MatchGroups("ac", &groups));

  graph = CompileInfix("a(?!(b))(b|c)", "ab((!bc|(..");
  REQUIRE(graph.MatchGroups("ac", &groups));
  REQUIRE("ac" == groups[0]);
  REQUIRE(groups[1].empty());
  REQUIRE("c" == groups[2]);

  REQUIRE_FALSE(graph.MatchGroups("ab", &groups));
}

TEST_CASE("graph possessive quantifiers", "[graph]") {
  auto graph = CompileInfix(".*+b", "_*+b.");
//  graph.DrawMermaid();
  REQUIRE_FALSE(graph.MatchGroups("b", nullptr));
  graph = CompileInfix("a*+b", "a*+b.");
  REQUIRE(4 == graph.MatchLen("aaab"));
  REQUIRE(4 == graph.MatchLen("aaab"));
  graph = CompileInfix(".*b", "_*b.");
  REQUIRE(graph.MatchGroups("aaab", nullptr));

  graph = CompileInfix("a?+a", "a?+a.");
//  graph.DrawMermaid();
  REQUIRE_FALSE(graph.MatchGroups("a", nullptr));
  REQUIRE(graph.MatchGroups("aa", nullptr));
}

TEST_CASE("graph {m,n}", "[graph]") {
  auto graph = CompileInfix("a{1,5}b", "a{1,5}b.");
  REQUIRE(2 == graph.MatchLen("ab"));
  REQUIRE(6 == graph.MatchLen("aaaaab"));
  REQUIRE(-1 == graph.MatchLen("b"));
  REQUIRE(-1 == graph.MatchLen("aaaaaab"));

  graph = CompileInfix("a{,1}b", "a{,1}b.");
  REQUIRE(1 == graph.MatchLen("b"));
  REQUIRE(2 == graph.MatchLen("ab"));
  REQUIRE(-1 == graph.MatchLen("aab"));

  graph = CompileInfix("a{1,}b", "a{1,}b.");
  REQUIRE(-1 == graph.MatchLen("b"));
  REQUIRE(2 == graph.MatchLen("ab"));

  graph = CompileInfix("a{,1}+a", "a{,1}+a.");
  REQUIRE(-1 == graph.MatchLen("a"));

  graph = CompileInfix("a{,1}?a", "a{,1}?a.");
  REQUIRE(1 == graph.MatchLen("aa"));

  graph = CompileInfix("a{2}", "a{2,2}");
  REQUIRE(-1 == graph.MatchLen("a"));
  REQUIRE(2 == graph.MatchLen("aa"));
  REQUIRE(2 == graph.MatchLen("aaa"));
}

TEST_CASE("graph back referencing", "[graph]") {
  auto graph = CompileInfix("a(?P<b>b)(?P<c>b|c)", "ab(<>bc|(<>..");
  std::vector<std::string> groups;
  REQUIRE(graph.MatchGroups("abc", &groups));
  REQUIRE("b" == groups[1]);
  REQUIRE("c" == groups[2]);
  auto matcher = graph.Match("abc");
  REQUIRE("b" == matcher.Group(1));
  REQUIRE("c" == matcher.Group(2));
  REQUIRE(matcher.ok());

  graph = CompileInfix("(?P<a>b|c)(?P=a)d", "bc|(<><1>d..");
//  graph.DrawMermaid();
  REQUIRE(-1 == graph.MatchLen("bcd"));
  REQUIRE(3 == graph.MatchLen("bbd"));
  REQUIRE(3 == graph.MatchLen("ccd"));
}

TEST_CASE("graph matcher", "[graph]") {
  std::vector<std::string> groups;
  auto graph = CompileInfix("a(?!(b))(b|c)", "ab((!bc|(..");
  REQUIRE(graph.MatchGroups("ac", &groups));
  REQUIRE("ac" == groups[0]);
  REQUIRE(groups[1].empty());
  REQUIRE("c" == groups[2]);

  REQUIRE_FALSE(graph.MatchGroups("ab", &groups));

  graph = CompileInfix("(?P<a>b|c)(?P=a)d", "bc|(<><1>d..");
  auto matcher = graph.Match("bbd");
  REQUIRE(matcher.ok());
  REQUIRE(3 == matcher.Group(0).size());
  REQUIRE("b" == matcher.Group(1));
  REQUIRE("b" == matcher.Group("a"));
}
