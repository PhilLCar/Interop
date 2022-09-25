#ifndef INTEROP_OBJECT_HEADER
#define INTEROP_OBJECT_HEADER

#include <vector>
#include <string>
#include <utility>

#include <prototype.h>
#include <enum.h>

namespace interop {
  using namespace std;

  struct Object {
  public:
    enum StructureType {
      UNION,
      STRUCTURE,
      CLASS
    };

  public:
    mutable Type::AccessLevel  currentAL = Type::PUBLIC;
    string                     type;
    string                     name;
    vector<Object>             objects;
    vector<Enum>               enums;
    vector<Prototype>          prototypes;
    vector<pair<Type, string>> members;
    vector<string>             knownTypes;
  };
}

#endif