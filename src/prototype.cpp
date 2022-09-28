#include <prototype.h>

namespace interop {
  string Prototype::toString() const {
    string s = return_type.toString();
    bool   f = true;

    s += " ";
    s += name;
    s += "(";

    for (vector<pair<Type, string>>::const_iterator it = params.begin(); it != params.end(); it++) {
      if (f) f = false;
      else   s += ", ";
      s += it->first.toString();
      if (it->second != "") {
        s += " ";
        s += it->second;
      }
    }
    s += ")";

    return s;
  }
}