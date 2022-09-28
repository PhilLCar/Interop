#pragma once

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
    Type::AccessLevel          al        = Type::PUBLIC;
    string                     file;
    string                     ns;
    string                     structure_type;
    string                     name;
    vector<Object>             objects;
    vector<Enum>               enums;
    vector<Prototype>          prototypes;
    vector<pair<Type, string>> members;
    vector<string>             knownTypes;
  };
}