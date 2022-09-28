#include <parser.h>

namespace interop {
  void Parser::parse(Tokenizer& tokenizer, Object& reference)
  {
    string s = tokenizer.peek();

    if (s == "namespace") {
      parseNamespace(tokenizer, reference);
    } else if (s == "using") {
      parseUsing(tokenizer, reference);
    } else if (s == "class" || s == "struct" || s == "union") {
      parseClass(tokenizer, reference);
    } else if (s == "private" || s == "protected" || s == "public") {
      parseAccess(tokenizer, reference);
    } else if (s == "enum") {
      parseEnum(tokenizer, reference);
    } else {
      // PROTOTYPE OR MEMBER
      pair<Type, string> m;
      Type& type   = m.first;
      string& name = m.second;

      type.al = reference.currentAL;

      parseType(tokenizer, reference, m);
      if (tokenizer.peek() == "(" || name == "operator") {
        // === Prototype === //
        Prototype pt;

        pt.return_type = type;
        pt.name        = name;
        parsePrototype(tokenizer, reference, pt);

        reference.prototypes.push_back(pt);
      } else {
        // === Member === //
        // Ignore member values (this is just a mask for the actual C++ object)
        tokenizer.seek(";");
        tokenizer.next();
        if (name != "") {
          // Members need to be named (assume forward declare if not)
          reference.members.push_back(m);
        }
      }

    }
  }

  void Parser::parseNamespace(Tokenizer& tokenizer, Object& reference)
  {
    string prev = reference.ns;

    tokenizer.expect("namespace");
    reference.ns = tokenizer.next();
    tokenizer.expect("{");

    while (tokenizer.peek() != "}" && tokenizer.peek() != "") parse(tokenizer, reference);
    tokenizer.next();

    reference.ns = prev;
  }

  void Parser::parseUsing(Tokenizer& tokenizer, Object& reference)
  {
    // Ingore the using statement for now
    tokenizer.expect("using");
    tokenizer.expect("namespace");
    tokenizer.next();
    if (tokenizer.peek() == "=") {
      tokenizer.next();
      tokenizer.next();
    }
    tokenizer.expect(";");
  }

  void Parser::parseClass(Tokenizer& tokenizer, Object& reference)
  {
    string s;
    Object obj;

    obj.structure_type = tokenizer.next();
    obj.file           = tokenizer.currentFile();
    obj.al             = reference.currentAL;

    for (vector<string>::const_iterator it = reference.knownTypes.begin(); it != reference.knownTypes.end(); it++) {
      obj.knownTypes.push_back(*it);
    }
    if (obj.structure_type == "class") {
      obj.currentAL = Type::PRIVATE;
    } else {
      obj.currentAL = Type::PUBLIC;
    }

    obj.name = "";
    for (s = tokenizer.peek(); s != "" && s != ";" && s != "{"; s = tokenizer.next()) {
      obj.name = s;
    }
    if (obj.name.length() > 0) {
      obj.knownTypes.push_back(obj.name);
      reference.knownTypes.push_back(obj.name);
    }

    // If not forward declaration
    if (s != ";" && s != "") {
      while (tokenizer.peek() != "}" && tokenizer.peek() != "") parse(tokenizer, obj);
      tokenizer.next();
      tokenizer.expect(";");

      reference.objects.push_back(obj);
    }
  }

  void Parser::parseAccess(Tokenizer& tokenizer, Object& reference)
  {
    string access = tokenizer.next();
    if      (access == "private")   reference.currentAL = Type::PRIVATE;
    else if (access == "protected") reference.currentAL = Type::PROTECTED;
    else  /*(access == "public")*/  reference.currentAL = Type::PUBLIC;
    tokenizer.expect(":");
  }

  void Parser::parseEnum(Tokenizer& tokenizer, Object& reference)
  {
    int    ct = 0;
    Enum   e;

    tokenizer.expect("enum");

    if (tokenizer.peek() != "{") {
      e.name = tokenizer.next();
      reference.knownTypes.push_back(e.name);
    } else e.name = ""; // Anonymous!
    tokenizer.expect("{");

    for (string s = tokenizer.next(); s != "}" && s != ""; s = tokenizer.next()) {
      pair<string, int> p;

      if (tokenizer.peek() == "=") {
        tokenizer.next();
        ct = stoi(tokenizer.next());
      }
      p.first = s;
      p.second = ct++;
      e.values.push_back(p);
      
      if (tokenizer.peek() == ",") tokenizer.next();
    }
    tokenizer.expect(";");

    reference.enums.push_back(e);
  }

  void Parser::parseType(Tokenizer& tokenizer, Object& reference, pair<Type, string>& m)
  {
    Type&   type = m.first;
    string& name = m.second;
    string  s;

    for (s = tokenizer.peek();; s = tokenizer.peek()) {
      if      (Type::isSpecifier(s.c_str())) type.specifier.push_back(s);
      else if (Type::isModifier(s.c_str()))  type.modifier.push_back(s);
      else if (Type::isQualifier(s.c_str())) type.qualifier.push_back(s);
      else break;
      tokenizer.next();
    }
    // Special cases: constructors and destructors don't have a type
    if (s == "~") {
      tokenizer.next();
      name      = s + tokenizer.next();
      type.name = "destructor";
    } else if (s == reference.name && (tokenizer.next(), tokenizer.peek()) == "(") {
      name      = s;
      type.name = "constructor";
    // The next value should be the type name
    } else {
      if (Type::isName(s.c_str()) || 
          find(reference.knownTypes.begin(), reference.knownTypes.end(), s) != 
          reference.knownTypes.end())
      {
        type.name = s;
        // This line is because of the consumed token in the constructor test
        if (type.name != reference.name) tokenizer.next();
      } else { // Implicit int
        type.name = "int";
      }
      for(s = tokenizer.peek();; s = tokenizer.peek()) {
        if      (s == "*")  type.storage.push_back(-1);
        else if (s == "&")  type.storage.push_back(-2);
        else if (s == "&&") type.storage.push_back(-3);
        else if (s == "const") /*ignore*/;
        else if (s == "[") {
          string tmp = tokenizer.next();

          if (tmp == "]") type.storage.push_back(0);
          else {
            type.storage.push_back(stoi(tmp));
            tokenizer.expect("]");
          }
        }
        else break;
        tokenizer.next();
      }
      // At this point we expect a variable name
      if (s == "(" || s == ";" || s == "=" || s == ",") name = "";
      else                                              name = tokenizer.next();
    }
  }

  void Parser::parsePrototype(Tokenizer& tokenizer, Object& reference, Prototype& pt)
  {
    if (pt.name == "operator") {
      string s = tokenizer.next();
      if      (s == "=")   pt.name = "_assign";
      else if (s == "==")  pt.name = "_eq";
      else if (s == "+")   pt.name = "_add";
      else if (s == "++")  pt.name = "_inc";
      else if (s == "+=")  pt.name = "_addeq";
      else if (s == "-")   pt.name = "_sub";
      else if (s == "--")  pt.name = "_dec";
      else if (s == "-=")  pt.name = "_subeq";
      else if (s == "*")   pt.name = "_mul";
      else if (s == "*=")  pt.name = "_muleq";
      else if (s == "/")   pt.name = "_div";
      else if (s == "/=")  pt.name = "_diveq";
      else if (s == "%")   pt.name = "_mod";
      else if (s == "%=")  pt.name = "_modeq";
      else if (s == "|")   pt.name = "_bor";
      else if (s == "|=")  pt.name = "_boreq";
      else if (s == "^")   pt.name = "_bxor";
      else if (s == "^=")  pt.name = "_bxoreq";
      else if (s == "&")   pt.name = "_band";
      else if (s == "&=")  pt.name = "_bandeq";
      else if (s == "||")  pt.name = "_lor";
      else if (s == "&&")  pt.name = "_land";
      else if (s == "<<")  pt.name = "_shl";
      else if (s == "<<=") pt.name = "_shleq";
      else if (s == ">>")  pt.name = "_shr";
      else if (s == ">>=") pt.name = "_shreq";
      else if (s == "!")   pt.name = "_not";
      else if (s == "!=")  pt.name = "_noteq";
      else if (s == "<")   pt.name = "_lt";
      else if (s == "<=")  pt.name = "_leq";
      else if (s == ">")   pt.name = "_gt";
      else if (s == ">=")  pt.name = "_geq";
      else if (s == "[")  {
        tokenizer.expect("]");
        pt.name = "at";
      }
      else if (s == "(")  {
        tokenizer.expect(")");
        pt.name = "call";
      }
    }
    tokenizer.expect("(");

    for (string s = tokenizer.peek(); s != ")" && s != ""; s = tokenizer.peek()) {
      const vector<const char*> stop = { ",", ")" };
      pair<Type, string> param;

      if (s == "...") {
        param.first.name = "varargs";
        tokenizer.next();
      } else {
        parseType(tokenizer, reference, param);
        pt.params.push_back(param);
      }
      // Discard default value, just consume until next parameter
      tokenizer.seek(stop);
      if (tokenizer.peek() == ",") tokenizer.next();
    }
    tokenizer.next();
    {
      const vector<const char*> stop = { ";", "{" };

      tokenizer.seek(stop);
      if (tokenizer.next() == "{") {
        tokenizer.seek("}");
        tokenizer.next();
      }
    }
  }
}