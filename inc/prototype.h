#pragma once

#include <string>
#include <vector>
#include <utility>

#include <type.h>

namespace interop {
  using namespace std;

  struct Prototype {
  public:
    friend string to_string(const Prototype& pt);

  public:
    Type                       return_type;
    string                     name;
    vector<pair<Type, string>> params;
  };
}