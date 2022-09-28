#pragma once

#include <string>
#include <vector>
#include <utility>

#include <type.h>

namespace interop {
  using namespace std;

  struct Prototype {
  public:
    string toString() const;

  public:
    Type                       return_type;
    string                     name;
    vector<pair<Type, string>> params;
  };
}