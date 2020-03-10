//
// Copyright [2020] <inhzus>
//

#include "regex/graph.h"

#include <catch2/catch.hpp>
#include <string>
#include <vector>

#include "test/utils.h"

inline regex::Graph
CompileInfix(const std::string &infix, const std::string &postfix) {
  auto exp = regex::Exp::FromStr(infix);
  REQUIRE(IdsToStr(exp.ids) == postfix);
  return regex::Graph::Compile(std::move(exp));
}

TEST_CASE("graph match concat") {
  auto graph = CompileInfix("aa", "aa.");
  REQUIRE(2 == graph.MatchLen("aa"));
  REQUIRE(2 == graph.MatchLen("aaa"));
  REQUIRE(-1 == graph.MatchLen("a"));
  REQUIRE(-1 == graph.MatchLen("b"));
  REQUIRE(-1 == graph.MatchLen(""));
}

TEST_CASE("graph match 0 or more") {
  auto graph = CompileInfix("aa*", "aa*.");
  REQUIRE(-1 == graph.MatchLen(""));
  REQUIRE(-1 == graph.MatchLen("b"));
  REQUIRE(1 == graph.MatchLen("a"));
  REQUIRE(2 == graph.MatchLen("aa"));
  REQUIRE(2 == graph.MatchLen("aab"));

  graph = CompileInfix("(a|b)*a", "ab|(*a.");
  REQUIRE(4 == graph.MatchLen("babac"));
}

TEST_CASE("graph match either one") {
  auto graph = CompileInfix("b|a*", "ba*|");
  REQUIRE(1 == graph.MatchLen("a"));
  REQUIRE(1 == graph.MatchLen("b"));
}

TEST_CASE("graph match the element or not") {
  auto graph = CompileInfix("a?b", "a?b.");
  REQUIRE(2 == graph.MatchLen("ab"));
  REQUIRE(1 == graph.MatchLen("b"));
  REQUIRE(-1 == graph.MatchLen("a"));
}

TEST_CASE("graph greedy or lazy") {
  auto graph = CompileInfix("aa*?", "aa*?.");
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

TEST_CASE("graph match any and backslash") {
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

TEST_CASE("graph match function that supports break backtrack") {
  auto graph = CompileInfix("ab", "ab.");
  REQUIRE(2 == graph.MatchLen("ab"));

  graph = CompileInfix("aa??", "aa??.");
  REQUIRE(1 == graph.MatchLen("aa"));

  graph = CompileInfix("aa*|b(cd*(e|fg))?h|i", "aa*.bcd*efg.|(..(?h..|i|");
  REQUIRE(8 == graph.MatchLen("bcdddfgh"));
  REQUIRE(1 == graph.MatchLen("i"));
  REQUIRE(2 == graph.MatchLen("bh"));
}

TEST_CASE("graph match groups") {
  auto graph = CompileInfix("aa*|b(cd*(e|fg))?h|i", "aa*.bcd*efg.|(..(?h..|i|");

  std::vector<std::string> groups;
  REQUIRE(graph.MatchGroups("bcdddfgh", &groups));
  REQUIRE(3 == groups.size());
  REQUIRE("bcdddfgh" == groups[0]);
  REQUIRE("cdddfg" == groups[1]);
  REQUIRE("fg" == groups[2]);
}

TEST_CASE("graph match non-captured groups") {
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

TEST_CASE("graph look-ahead") {
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

  graph = CompileInfix("a(?=(?P<ahead>foo|bar))", "afoo..bar..|(<>(=.");
  REQUIRE(graph.Match("abar").Group("ahead") == "bar");

  graph = CompileInfix("a(?!(b))(b|c)", "ab((!bc|(..");
  REQUIRE(graph.MatchGroups("ac", &groups));
  REQUIRE("ac" == groups[0]);
  REQUIRE(groups[1].empty());
  REQUIRE("c" == groups[2]);

  REQUIRE_FALSE(graph.MatchGroups("ab", &groups));
}

TEST_CASE("graph possessive quantifiers") {
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

TEST_CASE("graph {m,n}") {
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

TEST_CASE("graph back referencing") {
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

TEST_CASE("graph matcher") {
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

TEST_CASE("graph match atomic group") {
  auto graph = CompileInfix("(?>aa|a)a", "aa.a|(>a.");

  REQUIRE_FALSE(graph.Match("aa").ok());
  REQUIRE(graph.Match("aaa").ok());
}

TEST_CASE("graph match character set") {
  auto graph = CompileInfix("[ab]", "[1]");
  REQUIRE(graph.Match("a").ok());
  REQUIRE(graph.Match("b").ok());
  REQUIRE_FALSE(graph.Match("c").ok());
  REQUIRE_FALSE(graph.Match("").ok());

  graph = CompileInfix("[a-z]", "[1]");
  for (char ch = 'a'; ch <= 'z'; ++ch) {
    REQUIRE(graph.Match(std::string(1, ch)).ok());
  }

  graph = CompileInfix("[a-c-]", "[2]");
  for (char ch = 'a'; ch <= 'c'; ++ch) {
    REQUIRE(graph.Match(std::string(1, ch)).ok());
  }
  REQUIRE(graph.Match("-").ok());

  graph = CompileInfix("[-]", "[1]");
  REQUIRE(graph.Match("-").ok());

  graph = CompileInfix("[]]", "[1]");
  REQUIRE(graph.Match("]").ok());

  graph = CompileInfix("[^ab]", "[^1]");
  REQUIRE_FALSE(graph.Match("a"));
  REQUIRE_FALSE(graph.Match("b"));
  REQUIRE_FALSE(graph.Match(""));
  REQUIRE(graph.Match("c"));
}

TEST_CASE("graph match shorthand character class") {
#define CHECK_SHORTHAND_RANGE(out_of_range)  \
  {  \
    char ch = '\0';  \
    do {  \
      REQUIRE((graph.Match(std::string(1, ch)) || (out_of_range)));  \
    } while (++ch != '\0');  \
  }
  auto graph = CompileInfix("[\\d]", "[1]");
  CHECK_SHORTHAND_RANGE(ch < '0' || ch > '9');
  graph = CompileInfix("[\\D]", "[1]");
  CHECK_SHORTHAND_RANGE(ch >= '0' && ch <= '9');
  graph = CompileInfix("[\\s]", "[3]");  // [ \t\r\n\f]
  CHECK_SHORTHAND_RANGE(ch != ' ' && ch != '\t' &&
      ch != '\r' && ch != '\n' && ch != '\f');
  graph = CompileInfix("[\\S]", "[1]");
  CHECK_SHORTHAND_RANGE(ch == ' ' || ch == '\t' ||
      ch == '\r' || ch == '\n' || ch == '\f');
  graph = CompileInfix("[\\w]", "[4]");  // [A-Za-z0-9_]
  CHECK_SHORTHAND_RANGE((ch < 'A' || ch > 'Z') &&
      (ch < 'a' || ch > 'z') && (ch < '0' || ch > '9') && ch != '_');
  graph = CompileInfix("[\\W]", "[1]");
  CHECK_SHORTHAND_RANGE((ch >= 'A' && ch <= 'Z') ||
      (ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9') || ch == '_');
}

TEST_CASE("graph match end of the string") {
  auto graph = CompileInfix("a$", "a$.");
  REQUIRE(graph.Match("a"));
  REQUIRE_FALSE(graph.Match("aa"));
}
