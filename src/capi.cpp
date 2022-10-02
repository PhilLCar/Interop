#include <capi.h>

#include <iostream>

#include <tokenizer.h>
#include <parser.h>

namespace interop {
  CAPI::CAPI(const char* preprocessedFilename, const vector<const char*>& includePaths)
  {
    Tokenizer tokenizer(preprocessedFilename, includePaths);
    Parser::parse(tokenizer, root);
  }

  CAPI::~CAPI()
  {
  }

  void CAPI::makeC(const char* output) {
    string cpp_name = string(output) + ".cpp";
    string c_name   = string(output) + ".h";
    ofstream cpp;
    ofstream c;

    overloads.clear();
    cpp.open(cpp_name, ios::out);
    c.open(c_name, ios::out);

    printHeader(cpp);
    printHeader(c);
    printIncludes(cpp, root);
    c << "#pragma once" << endl << endl;
    cpp << endl << "extern \"C\" {" << endl;

    _makeC(c, cpp, root);

    cpp << "}" << endl;
  }

  void CAPI::_makeC(ofstream& c, ofstream& cpp, const Object& object)
  {
    for (vector<Object>::const_iterator it = object.objects.begin(); it != object.objects.end(); it++) _makeC(c, cpp, *it);

    for (vector<Prototype>::const_iterator it = object.prototypes.begin(); it != object.prototypes.end(); it++) {
      printPrototype(c, cpp, *it, object);
    }
  }

  bool CAPI::isEnum(const string& name, const Object& object)
  {
    for (vector<Enum>::const_iterator it = object.enums.begin(); it != object.enums.end(); it++) {
      if (it->name == name) return true;
    }
    for (vector<Object>::const_iterator it = object.objects.begin(); it != object.objects.end(); it++) {
      if (isEnum(name, *it)) return true;
    }
    return false;
  }

  bool CAPI::isOperator(const string& name)
  {
    return Tokenizer::OPERATORS.find(name) != Tokenizer::OPERATORS.end();
  }

  string CAPI::overload(const string& name)
  {
    int&   n  = overloads[name];
    string on = name;
    if (n++) on += to_string(n - 1);
    return on;
  }

  Prototype CAPI::expose(const Prototype& pt)
  {
    vector<pair<Type, string>>::const_iterator it = pt.params.begin();
    Prototype nPt;
    Type      tmp;

    for (int i = -2; i < (int)pt.params.size(); i++) {
      const Type* oldType;
      Type*       newType;

      if (i == -2) {
        oldType = &pt.return_type;
        newType = &nPt.return_type;
      } else if (i == -1) {
        if (pt.return_type.name != "constructor" &&
          find(pt.return_type.specifier.begin(), pt.return_type.specifier.end(), "static") 
          == pt.return_type.specifier.end()) {
          Type _type;
          _type.name = "void";
          _type.storage.push_back(-1);
          nPt.params.push_back({_type, "ref"});
        }
        continue;
      } else {
        tmp     = Type();
        oldType = &it++->first;
        newType = &tmp;
      }
      if (Type::isName(oldType->name.c_str())) {
        // Native type
        *newType = *oldType;
      } else if (isEnum(oldType->name, root)) {
        // Enum
        newType->name    = "int";
        newType->storage = oldType->storage;
      } else {
        // Any object
        newType->name    = "void";
        newType->storage = { -1 };
      }
      if (i >= 0) nPt.params.push_back({*newType, "arg" + to_string(i)});
    }

    if (pt.return_type.name == "constructor")     nPt.name = "construct";
    else if (pt.return_type.name == "destructor") nPt.name = "destruct";
    else if (isOperator(pt.name))                 nPt.name = Tokenizer::OPERATORS.find(pt.name)->second;
    else                                          nPt.name = pt.name;

    return nPt;
  }

  string CAPI::convarg(const pair<Type, string>& newPair, const pair<Type, string>& oldPair, string& body)
  {
    string arg  = "";
    bool   same = newPair.first.name == oldPair.first.name;
    const Type&  newType = newPair.first;
    const Type&  oldType = oldPair.first;

    for (vector<int>::const_iterator it = newType.storage.begin(), jt = oldType.storage.begin();
      same && it != newType.storage.end() && jt != oldType.storage.end(); it++, jt++)
    {
      same = *it == *jt;
    }

    if (same) {
      arg = newPair.second;
    } else {
      if (isEnum(oldType.name, root)) {
        // In the enum case a simple cast should suffice
        arg = "(" + oldType.name + ")" + newPair.second;
      } else { 
        // We know this is a custom type, because native types should be identical
        // And thus the argument should be "void*" in all cases
        if (find(oldType.storage.begin(), oldType.storage.end(), -2) != oldType.storage.end() ||
            find(oldType.storage.begin(), oldType.storage.end(), -3) != oldType.storage.end())
        {
          bool c = find(oldType.qualifier.begin(), oldType.qualifier.end(), "const") != oldType.qualifier.end();
          // If it's a reference, let the compiler do its thing
          arg = "t_" + newPair.second;
          body += "    " + to_string(oldType) + " " + arg + " = *(" + (c ? "const " : "") + oldType.name + "*)" + newPair.second + ";\n";
        } else {
          // If it's a pointer, simply cast
          arg = "(" + to_string(oldType) + ")" + newPair.second;
        }
      }
    }

    return arg;
  }

  string CAPI::funcName(const Prototype& pt, const Object& parent)
  {
    string ret = to_string(pt.return_type) + " " + overload(parent.name + "_" + pt.name) + "(";

    for (vector<pair<Type, string>>::const_iterator it = pt.params.begin(); it != pt.params.end(); ) {
      ret += to_string(it->first) + " " + it->second;
      if (++it != pt.params.end()) ret += ", ";
    }

    return ret + ")";
  }

  string CAPI::oper(const Prototype& newPt, const Prototype& oldPt, const vector<string>& args, const Object& parent)
  {
    string body = "    " + parent.name + "& t_ref = *(" + parent.name + "*)ref;\n";
    string ref;

    body += "    return ";
    if (find(oldPt.return_type.storage.begin(), oldPt.return_type.storage.end(), -2) != oldPt.return_type.storage.end()) {
      ref = "(void*)&(t_ref";
    } else {
      ref = "(" + to_string(newPt.return_type) + ")(t_ref";
    }
    if (oldPt.name == "(") {
      // Call special case
      body += ref + "(";
      for (vector<string>::const_iterator it = args.begin(); it != args.end(); ) {
        body += *it;
        if (++it != args.end()) body += ", ";
      }
      body += "));\n";
    } else if (oldPt.name == "[") {
      // Index special case
      body += ref + "[" + args[0] + "]);\n";
    } else if (args.size() == 0) {
      // Unary operator
      body += oldPt.name + ref + ");\n";
    } else if (args.size() == 1) {
      // Binary operator
      body += ref + " " + oldPt.name + " " + args[0] + ");\n";
    } else {
      // Ternary operators are ignored for now
    }

    return body;
  }

  void CAPI::printPrototype(ofstream& c, ofstream& cpp, const Prototype& pt, const Object& parent)
  {
    Prototype      nPt   = expose(pt);
    string         body  = "";
    string         fname = funcName(nPt, parent);
    vector<string> args;

    cpp << "  " << fname << endl << "  {" << endl;
    c << "extern " << fname << ";" << endl;

    for (vector<pair<Type, string>>::const_iterator it = pt.params.begin(), jt = nPt.params.begin();
         it != pt.params.end() && jt != nPt.params.end(); it++, jt++)
    {
      if (jt->second == "ref") ++jt;
      args.push_back(convarg(*jt, *it, body));
    }

    { // Return
      bool arg = true;
      const Type& oldType = pt.return_type;
      const Type& newType = nPt.return_type;

      if (oldType.name == "constructor") {
        body += "    return (void*)new " + parent.name + "(";
      } else if (oldType.name == "destructor") {
        body += "    delete (" + parent.name + "*)ref;\n";
        arg = false;
      } else if (find(oldType.specifier.begin(), oldType.specifier.end(), "static") != oldType.specifier.end()) {
        if (find(oldType.storage.begin(), oldType.storage.end(), -2) != oldType.storage.end()) body += "    return (void*)&";
        else if (oldType.name == "void" && oldType.storage.size() == 0) body += "    ";
        else body += "    return ";
        body += parent.name + "::" + nPt.name + "(";
      } else if (isOperator(pt.name)) {
        body += oper(nPt, pt, args, parent);
        arg = false;
      } else {
        if (find(oldType.storage.begin(), oldType.storage.end(), -2) != oldType.storage.end()) body += "    return (void*)&";
        else if (oldType.name == "void" && oldType.storage.size() == 0) body += "    ";
        else body += "    return ";
        body += "((" + parent.name + "*)ref)->" + nPt.name + "(";
      }

      if (arg) {
        for (vector<string>::const_iterator it = args.begin(); it != args.end(); ) {
          body += *it;
          if (++it != args.end()) body += ", ";
        }
        body += ");\n";
      }
    }

    cpp << body << "  }" << endl << endl;
  }

  void CAPI::printIncludes(ofstream& cpp, Object& root)
  {
    for (vector<Object>::const_iterator it = root.objects.begin(); it != root.objects.end(); it++) {
      string s = "";

      for (const char *c = &it->file[0]; *c; c++) {
        if (*c == '\\') s += *c;
        s += *c;
      }
      cpp << "#include \"" << s << "\"" << endl;
    }
  }

  void CAPI::printHeader(ofstream& out)
  {
    out << "/////////////////////////////////////////////////////////////////////////////////////////////////" << endl;
    out << "// This document was programatically generated by the C Automated Programming Interface (CAPI) //" << endl;
    out << "// For more information, visit: http://www.github.com/PhilLCar/CppInteropUtils                 //" << endl;
    out << "/////////////////////////////////////////////////////////////////////////////////////////////////" << endl;
    out << endl;
  }
}