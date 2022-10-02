#include <type.h>

namespace interop {
  string to_string(const Type& type) {
    string s = type.name;
    
    if (find(type.qualifier.begin(), type.qualifier.end(), "const") != type.qualifier.end()) s = "const " + s;
    for (vector<int>::const_iterator it = type.storage.begin(); it != type.storage.end(); it++) {
      switch (*it) {
      case 0:
        s += "[]";
        break;
      case -1:
        s += "*";
        break;
      case -2:
        s += "&";
        break;
      case -3:
        s += "&&";
        break;
      default:
        s += string("[") + std::to_string(*it) + "]";
        break;
      }
    }
    return s;
  }
}