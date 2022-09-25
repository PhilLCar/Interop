#ifndef INTEROP_ENUM_HEADER
#define INTEROP_ENUM_HEADER

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

#endif