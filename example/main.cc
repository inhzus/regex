//
// Copyright [2020] <inhzus>
//

#include <cstdio>

#include "regex/graph.h"

int main() {
  auto exp = regex::Exp::FromStr("a(\\w)(?P<name>d|e)(?P=name)");
  auto pattern = regex::Graph::Compile(std::move(exp));
  auto match = pattern.Match("ba_dd");
  if (!match) {
    printf("not match\n");
    return 0;
  }
  printf("match: %.*s\n", static_cast<int>(match.Size()),
         match.Group(0).data());
  printf("<1>: %.*s\n", static_cast<int>(match.Group(1).size()),
         match.Group(1).data());
  printf("<name>: %.*s\n", static_cast<int>(match.Group("name").size()),
         match.Group("name").data());
  return 0;
}

