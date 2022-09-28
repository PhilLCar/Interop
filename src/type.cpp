#include <type.h>

namespace interop {
  string Type::toString() const {
    string s = name;
    for (vector<int>::const_iterator it = storage.begin(); it != storage.end(); it++) {
      switch (*it) {
      case 0:
        s += "[]";
        break;
      case -1:
        s += "*";
        break;
      case -2:
        s += "*";
        break;
      case -3:
        s += "*";
        break;
      default:
        s += string("[") + to_string(*it) + "]";
        break;
      }
    }
    return s;
  }
}