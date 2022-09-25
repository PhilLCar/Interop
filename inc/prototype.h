#ifndef INTEROP_PROTOTYPE_HEADER
#define INTEROP_PROTOTYPE_HEADER

#include <string>
#include <vector>
#include <utility>

#include <type.h>

namespace interop {
  using namespace std;

  struct Prototype {
  public:
    Type                       return_type;
    string                     name;
    vector<pair<Type, string>> params;
  };
}

#endif