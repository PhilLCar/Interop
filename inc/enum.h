#pragma once

#include <vector>
#include <string>
#include <utility>

namespace interop {
  using namespace std;

  struct Enum {
  public:
    string                    name;
    vector<pair<string, int>> values;
  };
}