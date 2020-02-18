//
// Copyright [2020] <inhzus>
//

#include "regex/graph.h"

#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>

TEST_CASE("graph match concat", "[graph]") {
  auto segment = regex::Graph::Compile("aa.");
  REQUIRE(2 == segment.Match("aa"));
  REQUIRE(2 == segment.Match("aaa"));
  REQUIRE(-1 == segment.Match("a"));
  REQUIRE(-1 == segment.Match("b"));
  REQUIRE(-1 == segment.Match(""));
}

TEST_CASE("graph match 0 or more", "[graph]") {
  auto graph = regex::Graph::Compile("a*");

  REQUIRE(0 == graph.Match(""));
  REQUIRE(0 == graph.Match("b"));
  REQUIRE(1 == graph.Match("a"));
  REQUIRE(2 == graph.Match("aa"));
  REQUIRE(2 == graph.Match("aab"));
}
