#include <capi.h>

#include <iostream>
#include <stdexcept>

#include <cstring>

#ifdef WIN
#define SLASH '\\'
#else
#define SLASH '/'
#endif


// TODO: This is a quick and dirty implementaion
// With time it should be modularized (ex: seperate the tokenizer "next" from the parser "parse") and formalized

namespace interop {
  CAPI::CAPI(const char* preprocessed, const vector<const char*>& include)
    : UNMERGEABLE{ ";", ":", ",", "{", "}", "(", ")", "[", "]", "<", "<<",
                   "<=", "<<=", ">", ">>", ">=", ">>=", "+", "++", "+=", 
                   "-", "--", "-=", ".", "~", "*", "*=", "/", "/=", "&", 
                   "&&", "&=", "^", "^", "^=", "|", "||", "|=", "!", "!=",
                   "%", "%=", "?", "==", "//", "..." }
    , WHITESPACE{ ' ', '\t', '\n', '\r' }
    , INCLUDE(include)
    , buffer{0}
    , ns("")
  {
    ifstream fileStream;
    int      line = 0;

    fileStream.open(preprocessed);

    for (int i = 0; i < CAPI_BUFLENGTH; i++) buffer[i] = fileStream.get();

    for (string s = next(fileStream); s != ""; s = next(fileStream) ) {
      if (!checkin(fileStream, s)) continue;
      parse(fileStream, s);
    }

    fileStream.close();
  }

  CAPI::~CAPI()
  {
  }

  void CAPI::makeC(const char* output) {
    Object a = objects[0];
    for (int i = 0; i < a.members.size(); i++) {
      cout << a.members[i].first.name << " " << a.members[i].second << endl;
    }
  }

  void CAPI::parseMain(ifstream& fileStream, string& s, Object* ref)
  {
    if (s[0] == '#') {
      ignore(fileStream);
    } else if (s == "namespace") {
      parseNamespace(fileStream);
    } else if (s == "using") {
      parseUsing(fileStream);
    } else if (s == "class" || s == "struct" || s == "union") {
      parseClass(fileStream, s, ref);
    } else if (s == "private" || s == "protected" || s == "public") {
      parseAccess(fileStream, s, ref);
    } else if (s == "enum") {
      parseEnum(fileStream, ref);
    } else {
      // PROTOTYPE OR MEMBER
      pair<Type, string> m;

      if (ref != nullptr) m.first.al = ref->currentAL;
      else                m.first.al = Type::PUBLIC;

      parseType(fileStream, s, m, ref);
      if (s == "(" || m.second == "operator") { // === Prototype === //
        Prototype pt;

        pt.return_type = m.first;
        pt.name        = m.second;
        parsePrototype(fileStream, s, pt, ref);

        if (ref != nullptr) ref->prototypes.push_back(pt);
        else                prototypes.push_back(pt);
      } else { // === Member === //
        // Ignore member values (this is just a mask for the actual C++ object)
        if (s == "=") do { s = next(fileStream); } while (s != "" && s != ";");
        if (m.second != "") { // Members need to be named (assume forward declare if not)
          if (ref != nullptr) ref->members.push_back(m);
          else                members.push_back(m);
        }
      }

    }
  }

  void CAPI::parseNamespace(ifstream& fileStream, Object* ref)
  {
    string s;
    string prev = ns;

    ns = next(fileStream);
    expect(fileStream, "{");
    while ((s = next(fileStream)) != "}" && s != "") parseMain(fileStream, s, ref);

    ns = prev;
  }

  void CAPI::parseUsing(ifstream& fileStream, Object *ref)
  {
    expect(fileStream, "namespace");
    // Ingore the using statement for now
    next(fileStream);
    if (next(fileStream) == "=") {
      next(fileStream);
      expect(fileStream, ";");
    }
  }

  void CAPI::parseClass(ifstream& fileStream, string& type, Object *ref)
  {
    string prev = next(fileStream);
    string s;
    Object obj;

    obj.type = type;
    if (ref != nullptr) {
      for (vector<string>::const_iterator it = ref->knownTypes.begin(); it != ref->knownTypes.end(); it++) 
        obj.knownTypes.push_back(*it);
    } else {
      for (vector<string>::const_iterator it = knownTypes.begin(); it != knownTypes.end(); it++) 
        obj.knownTypes.push_back(*it);
    }
    if (type == "class") {
      obj.currentAL = Type::PRIVATE;
    } else {
      obj.currentAL = Type::PUBLIC;
    }

    if (prev == "{") {
      obj.name = ""; // Anonymous!
    } else {
      for (s = next(fileStream); s != "{" && s != ";" && s != ""; prev = s, s = next(fileStream));
      obj.name = prev;
      obj.knownTypes.push_back(prev);
    }

    if (s != ";" && s != "") { // If not forward declaration
      while ((s = next(fileStream)) != "}" && s != "") parseMain(fileStream, s, &obj);

      if (ref != nullptr) ref->objects.push_back(obj);
      else                objects.push_back(obj);

      expect(fileStream, ";");
    }
    if (obj.name != "") {
      if (ref != nullptr) ref->knownTypes.push_back(obj.name);
      else                knownTypes.push_back(obj.name);
    }
  }

  void CAPI::parseAccess(ifstream& fileStream, string& access, Object* ref)
  {
    if (ref == nullptr) throw exception("Unexpected access specifier in main body");
    if      (access == "private")   ref->currentAL = Type::PRIVATE;
    else if (access == "protected") ref->currentAL = Type::PROTECTED;
    else  /*(access == "public")*/  ref->currentAL = Type::PUBLIC;
    expect(fileStream, ":");
  }

  void CAPI::parseEnum(ifstream& fileStream, Object* ref)
  {
    int    ct   = 0;
    string name = next(fileStream);
    Enum e;

    if (name != "{") {
      e.name = name;
      expect(fileStream, "{");
    } else {
      e.name = ""; // Anonymous!
    }

    for (string s = next(fileStream); s != "}" && s != ""; s == "," ? s = next(fileStream) : s) {
      pair<string, int> p;
      p.first = s;
      if ((s = next(fileStream)) == "=") {
        ct = stoi(next(fileStream));
        s = next(fileStream);
      }
      p.second = ct++;
      e.values.push_back(p);
    }
    expect(fileStream, ";");

    if (ref != nullptr) {
      ref->enums.push_back(e);
      ref->knownTypes.push_back(e.name);
    } else {
      enums.push_back(e);
      knownTypes.push_back(e.name);
    }
  }

  void CAPI::parseType(ifstream& fileStream, string& s, pair<Type, string>& m, Object* ref)
  {
    Type&   type = m.first;
    string& name = m.second;
    bool    peek = false;

    while (true) {
      if (Type::isSpecifier(s.c_str()))      type.specifier.push_back(s);
      else if (Type::isModifier(s.c_str()))  type.modifier.push_back(s);
      else if (Type::isQualifier(s.c_str())) type.qualifier.push_back(s);
      else break;
      s = next(fileStream);
    }
    // Special case: constructors and destructors don't have a type;
    if (s == "~" || s == ref->name) {
      // TODO: Refactor: this is disgusting
      if (s == "~") {
        name = s + next(fileStream);
        type.name = "destructor";
        s = next(fileStream);
        return;
      } else if ((s = next(fileStream)) == "(") {
        name = ref->name;
        type.name = "constructor";
        return;
      } else peek = true;
    }
    // The next value should be the type name
    {
      vector<string>& kt = ref != nullptr ? ref->knownTypes : knownTypes;
      
      if (Type::isName(s.c_str()) || find(kt.begin(), kt.end(), s) != kt.end()) {
        type.name = s;
        if (!peek) s = next(fileStream);
      } else { // Implicit int
        type.name = "int";
      }
    }
    while (true) {
      if (s == "*") type.storage.push_back(-1);
      else if (s == "&") type.storage.push_back(-2);
      else if (s == "&&") type.storage.push_back(-3);
      else if (s == "const") /*ignore*/;
      else if (s == "[") {
        string tmp = next(fileStream);

        if (tmp == "]") type.storage.push_back(-1);
        else {
          type.storage.push_back(stoi(tmp));
          expect(fileStream, "]");
        }
      }
      else break;
      s = next(fileStream);
    }
    // At this point we expect a variable name
    if (s == "(" || s == ";" || s == "=") {
      // No name: either a forward declaration or a parameter
      name = "";
    } else {
      name = s;
      s = next(fileStream);
    }
  }

  void CAPI::parsePrototype(ifstream& fileStream, string& s, Prototype& pt, Object* ref)
  {
    if (pt.name == "operator") {
      if      (s == "=")   pt.name = "eq";
      else if (s == "+")   pt.name = "add";
      else if (s == "++")  pt.name = "inc";
      else if (s == "+=")  pt.name = "addeq";
      else if (s == "-")   pt.name = "sub";
      else if (s == "--")  pt.name = "dec";
      else if (s == "-=")  pt.name = "subeq";
      else if (s == "*")   pt.name = "mul";
      else if (s == "*=")  pt.name = "muleq";
      else if (s == "/")   pt.name = "div";
      else if (s == "/=")  pt.name = "diveq";
      else if (s == "%")   pt.name = "mod";
      else if (s == "%=")  pt.name = "modeq";
      else if (s == "|")   pt.name = "bor";
      else if (s == "|=")  pt.name = "boreq";
      else if (s == "^")   pt.name = "bxor";
      else if (s == "^=")  pt.name = "bxoreq";
      else if (s == "&")   pt.name = "band";
      else if (s == "&=")  pt.name = "bandeq";
      else if (s == "||")  pt.name = "lor";
      else if (s == "&&")  pt.name = "land";
      else if (s == "<<")  pt.name = "shl";
      else if (s == "<<=") pt.name = "shleq";
      else if (s == ">>")  pt.name = "shr";
      else if (s == ">>=") pt.name = "shreq";
      else if (s == "!")   pt.name = "not";
      else if (s == "!=")  pt.name = "noteq";
      else if (s == "[")  {
        expect(fileStream, "]");
        pt.name = "at";
      }
      else if (s == "(")  {
        expect(fileStream, ")");
        pt.name = "call";
      }
    }

    for (s = next(fileStream); s != ")" && s != ""; ) {
      const vector<string> STOP = { ",", ")" };
      pair<Type, string>   param;

      if (s == "...") param.first.name = "varargs";
      else            parseType(fileStream, s, param, ref);
      pt.params.push_back(param);
      // Discard default value, just consume until next parameter
      discard(fileStream, s, STOP);
    }
    {
      const vector<string> STOP = { ";", "}" };

      s = next(fileStream);
      // Discard function body
      discard(fileStream, s, STOP);
    }
  }

  void CAPI::parse(ifstream& fileStream, string& s) {
    while (true) {
      if (s == "#line") {
        if (!checkin(fileStream, s)) break;
      } else if (s[0] == '#') {
        ignore(fileStream);
        s = next(fileStream);
      } else {
        parseMain(fileStream, s);
        s = next(fileStream);
      }
    }
  }

  string CAPI::next(ifstream& fileStream)
  {
    string accumulator = "";
    int    match;

    while (buffer[0] != EOF) {
      if ((match = unmergeable()) > 0) {
        if (accumulator.length() == 0) {
          for (int i = 0; i < match; i++) {
            accumulator += buffer[0];
            for (int j = 0; j < CAPI_BUFLENGTH - 1; j++) {
              buffer[j] = buffer[j + 1];
            }
            buffer[CAPI_BUFLENGTH - 1] = fileStream.get();
          }
        }
        break;
      } else if (whitespace()) {
        if (accumulator.length() != 0) break;
      } else if (buffer[0] == '\"' || (buffer[0] == '\'' && 
                 (accumulator.length() == 0 || // Catches weird unsigned long long notation
                 (accumulator.length() >= 2 && accumulator[0] == '0' && accumulator[1] == 'x')))) {
        if (accumulator.length() == 0) accumulator = charseq(fileStream);
        break;
      } else {
        accumulator += buffer[0];
      }
      for (int j = 0; j < CAPI_BUFLENGTH - 1; j++) {
        buffer[j] = buffer[j + 1];
      }
      buffer[CAPI_BUFLENGTH - 1] = fileStream.get();
    }

    return accumulator;
  }

  string CAPI::charseq(ifstream& fileStream)
  {
    char   terminator = buffer[0];
    string sequence   = string("") + terminator;
    int    used       = 1;

    for (; used < CAPI_BUFLENGTH && buffer[used] != terminator; used++) {
      if (buffer[used] == '\\' && ++used == CAPI_BUFLENGTH) sequence += fileStream.get();
      else sequence += buffer[used];
    }
    // If the buffer is completely used, means string still going
    if (used == CAPI_BUFLENGTH) {
      for (char c = fileStream.get(); c!= terminator && c!= EOF; c = fileStream.get()) {
        if (c == '\\') c = fileStream.get();
        sequence += c;
      }
    } else ++used;
    sequence += terminator;

    {
      int remainder = CAPI_BUFLENGTH - used;
      for (int i = 0; i < remainder; i++) buffer[i]             = buffer[i + used];
      for (int i = 0; i < used; i++)      buffer[i + remainder] = fileStream.get();
    }

    return sequence;
  }

  void CAPI::ignore(ifstream& fileStream) {
    for (char c = 0; c != EOF && c != '\n'; c = fileStream.get());
    for (int i = 0; i < CAPI_BUFLENGTH; i++) buffer[i] = fileStream.get();
  }

  void CAPI::discard(ifstream& fileStream, string& s, const vector<string>& until) {
    int parent  = 0;
    int bracket = 0;
    int accolad = 0;
    for (; s != ""; s = next(fileStream)) {
      if      (s == "(") parent++;
      else if (s == ")") parent--;
      else if (s == "[") bracket++;
      else if (s == "]") bracket--;
      else if (s == "{") accolad++;
      else if (s == "}") accolad--;
      if ((parent + bracket + accolad) <= 0 && find(until.begin(), until.end(), s) != until.end()) break;
    }
  }

  void CAPI::expect(ifstream& fileStream, const char* what)
  {
    string except = string("Ill formed expression, expected: ") + what;
    
    if (next(fileStream) != what) throw exception(except.c_str());
  }

  int CAPI::unmergeable()
  {
    int max = 0;

    for (vector<const char*>::const_iterator tmp = UNMERGEABLE.begin(); tmp != UNMERGEABLE.end(); tmp++) {
      int len = (int)strlen(*tmp);

      for (int i = 0; i < len; i++) {
        if (buffer[i] != (*tmp)[i]) len = -1;
      }

      if (len > max) max = len;
    }

    return max;
  }

  bool CAPI::whitespace()
  {
    for (int i = 0; i < WHITESPACE.size(); i++) {
      if (buffer[0] == WHITESPACE[i]) return true;
    }
    return false;
  }

  bool CAPI::included(string &s)
  {
    string path = "";
    string fold = "";
    bool   in   = false;

    for (int i = 1; i < s.length() - 1; i++) {
      if (s[i] == SLASH) {
        path += fold + s[i];
        fold = "";
      } else fold += s[i];
    }
    for (vector<const char*>::const_iterator inc = INCLUDE.begin(); inc != INCLUDE.end() && !in; inc++) {
      in = path == *inc;
    }

    return in;
  }

  bool CAPI::checkin(ifstream& fileStream, string& str)
  {
    bool ok = false;

    if (str == "#line") {
      next(fileStream); // should be a number, skip it
      if (included((str = next(fileStream)))) ok = true;
      str = next(fileStream);
    }
    return ok;
  }
}