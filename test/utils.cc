//
// Copyright [2020] <inhzus>
//

#include "test/utils.h"

#include <cassert>

using regex::Id;

std::string IdsToStr(const std::vector<Id> &vector) {
  std::string s;
  for (const auto &id : vector) {
    switch (static_cast<int>(id.sym)) {
      case Id::Sym::AheadPr: s += "(=";
        break;
      case Id::Sym::NegAheadPr: s += "(!";
        break;
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
      case Id::Sym::PosMore: s += "*+";
        break;
      case Id::Sym::RelMore: s += "*?";
        break;
      case Id::Sym::Paren: s.push_back('(');
        break;
      case Id::Sym::UnParen: break;
      case Id::Sym::ParenEnd: s.push_back(')');
        break;
      case Id::Sym::Quest: s.push_back('?');
        break;
      case Id::Sym::RelQuest: s += "??";
        break;
      default:assert(false);
        break;
    }
  }
  return s;
}
