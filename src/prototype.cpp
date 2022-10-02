#include <prototype.h>

namespace interop {
  string to_string(const Prototype& pt) {
    string s = to_string(pt.return_type);
    bool   f = true;

    s += " ";
    s += pt.name;
    s += "(";

    for (vector<pair<Type, string>>::const_iterator it = pt.params.begin(); it != pt.params.end(); it++) {
      if (f) f = false;
      else   s += ", ";
      s += to_string(it->first);
      if (it->second != "") {
        s += " ";
        s += it->second;
      }
    }
    s += ")";

    return s;
  }
}