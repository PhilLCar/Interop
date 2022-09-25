#ifndef INTEROP_CAPI_HEADER
#define INTEROP_CAPI_HEADER

#include <vector>
#include <fstream>

#include <object.h>
#include <prototype.h>
#include <enum.h>

#define CAPI_BUFLENGTH 3

namespace interop {
  using namespace std;

  class CAPI {
    // Exposes a C++ class in a C format to be included in a C project/library
  public:
    CAPI(const char* preprocessed, const vector<const char*>& include);
    ~CAPI();

  public:
    void makeC(const char* output);
    void makeCS(const char* output);

  private:
    void parseMain(ifstream& fileStream, string& s, Object* ref = nullptr);
    void parseNamespace(ifstream& fileStream, Object* ref = nullptr);
    void parseUsing(ifstream& fileStream, Object* ref = nullptr);
    void parseClass(ifstream& fileStream, string& type, Object* ref = nullptr);
    void parseAccess(ifstream& fileStream, string& access, Object* ref = nullptr);
    void parseEnum(ifstream& fileStream, Object* ref = nullptr);
    void parseType(ifstream& fileStream, string& s, pair<Type, string>& p, Object* ref = nullptr);
    void parsePrototype(ifstream& fileStream, string& s, Prototype& pt, Object* ref = nullptr);

    void   parse(ifstream& fileStream, string& string);
    string next(ifstream& fileStream);
    string charseq(ifstream& fileStream);
    void   ignore(ifstream& fileStream);
    void   discard(ifstream& fileStream, string& s, const vector<string>& until);
    void   expect(ifstream& fileStream, const char* what);

    int    unmergeable();
    bool   whitespace();
    bool   included(string& s);
    bool   checkin(ifstream& fileStream, string& str);

  private:
    string                     ns;
    vector<Object>             objects;
    vector<Enum>               enums;
    vector<Prototype>          prototypes;
    vector<pair<Type, string>> members;
    vector<string>             knownTypes;
    char                       buffer[CAPI_BUFLENGTH + 1];

    const vector<const char*>  UNMERGEABLE;
    const vector<char>         WHITESPACE;
    const vector<const char*>& INCLUDE;
  };
}

#endif