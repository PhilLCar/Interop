#ifndef INTEROP_TYPE_HEADER
#define INTEROP_TYPE_HEADER

#include <string>
#include <vector>

#include <cstring>

namespace interop {
  using namespace std;

  struct Type {
  public:
    enum AccessLevel {
      PRIVATE,
      PROTECTED,
      PUBLIC
    };

  private:
    static constexpr int         TYPENAMES_SIZE               = 7;
    static constexpr const char* TYPENAMES[TYPENAMES_SIZE]    = { "bool", "char", "int", "float", "double", "void", "auto" };
    static constexpr int         MODIFIERS_SIZE               = 5; // "class" is not really a modifer, but is used similarly during friend declarations
    static constexpr const char* MODIFIERS[MODIFIERS_SIZE]    = { "singed", "unsigned", "short", "long", "class" };
    static constexpr int         QUALIFIERS_SIZE              = 6;
    static constexpr const char* QUALIFIERS[QUALIFIERS_SIZE]  = { "const", "mutable", "volatile", "restrict", "constexpr", "noexcept" };
    static constexpr int         SPECIFIERS_SIZE              = 6;
    static constexpr const char* SPECIFIERS[SPECIFIERS_SIZE]  = { "inline", "extern", "virtual", "static", "friend", "explicit" };

  private:
    static inline bool in(const char *const*list, int size, const char* expr) {
      for (int i = 0; i < size; i++) if (!strcmp(list[i], expr)) return true;
      return false;
    }
  public:
    static inline bool isName(const char* expr)      { return in(TYPENAMES,  TYPENAMES_SIZE,  expr); }
    static inline bool isModifier(const char* expr)  { return in(MODIFIERS,  MODIFIERS_SIZE,  expr); }
    static inline bool isQualifier(const char* expr) { return in(QUALIFIERS, QUALIFIERS_SIZE, expr); }
    static inline bool isSpecifier(const char* expr) { return in(SPECIFIERS, SPECIFIERS_SIZE, expr); }

  public:
    AccessLevel    al = Type::PUBLIC;
    string         name;
    vector<string> modifier;
    vector<string> qualifier;
    vector<string> specifier;
    vector<int>    storage;
  };
}

#endif