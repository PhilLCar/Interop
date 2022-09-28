#pragma once

#include <fstream>

#include <object.h>

#define CAPI_BUFLENGTH 3

namespace interop {
  using namespace std;

  class CAPI {
    // Exposes a C++ class in a C format to be included in a C project/library
  public:
    CAPI(const char* preprocessedFilename, const vector<const char*>& includePaths);
    ~CAPI();

  public:
    void makeC(const char* output);
    void makeCS(const char* output);

  private:
    void _makeC(ofstream& c, ofstream& cpp, const Object& object);
    void _includeC(ofstream& cpp, const Object& object);
    void printHeader(ofstream& out);

  private:
    Object root;
  };
}
