#pragma once

#include <tokenizer.h>
#include <object.h>


namespace interop {
  using namespace std;

  class Parser {
  public:
    static void parse(Tokenizer& tokenizer, Object& reference);
  private:
    static void parseNamespace(Tokenizer& tokenizer, Object& reference);
    static void parseUsing(Tokenizer& tokenizer, Object& reference);
    static void parseClass(Tokenizer& tokenizer, Object& reference);
    static void parseAccess(Tokenizer& tokenizer, Object& reference);
    static void parseEnum(Tokenizer& tokenizer, Object& reference);
    static void parseType(Tokenizer& tokenizer, Object& reference, pair<Type, string>& p);
    static void parsePrototype(Tokenizer& tokenizer, Object& reference, Prototype& pt);
  };
}