#pragma once

#include <fstream>
#include <map>

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

    bool isEnum(const string& name, const Object& root);
    bool isOperator(const string& name);

    string    overload(const string& name);
    Prototype expose(const Prototype& pt);
    string    convarg(const pair<Type, string>& newPair, const pair<Type, string>& oldPair, string& body);
    string    funcName(const Prototype& pt, const Object& parent);
    string    oper(const Prototype& newPt, const Prototype& oldPt, const vector<string>& args, const Object& parent);
    
    void printPrototype(ofstream& c, ofstream& cpp, const Prototype& pt, const Object& parent);

    void printIncludes(ofstream& cpp, Object& object);
    void printHeader(ofstream& out);

  private:
    Object           root;
    map<string, int> overloads;
  };
}
