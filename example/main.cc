//
// Copyright [2020] <inhzus>
//

#include <fmt/format.h>

#include <cstdio>

#include "regex/graph.h"

int main() {
  auto pattern = regex::Graph::Compile("a(\\w)(?P<name>d|e)(?P=name)");
  auto match = pattern.Match("ba_dd");
  if (!match) {
    fmt::print("not match\n");
    return 0;
  }
  fmt::print("match: {}\n", match.Str());           // expected: "a_dd"
  fmt::print("<1>: {}\n", match.Group(1));          // "_"
  fmt::print("<name>: {}\n", match.Group("name"));  // "d"

  // subroutine
  pattern = regex::Graph::Compile("a(b)(?P<c>c)");
  auto s = pattern.Sub("\\g<c>\\1", "abcdeabc");
  fmt::print("sub: {}\n", s);  // expected: "cbdecb"
  return 0;
}

