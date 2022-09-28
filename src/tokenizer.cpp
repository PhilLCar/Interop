#include <tokenizer.h>

#include <stdexcept>
#include <cstring>

#ifdef WIN
#define SLASH '\\'
#else
#define SLASH '/'
#endif

namespace interop {
  const vector<char>        Tokenizer::WHITESPACE  = { ' ', '\t', '\n', '\r' };
  const vector<const char*> Tokenizer::UNMERGEABLE = { ";", ":", ",", "{", "}", "(", ")", "[", "]", "<", "<<",
                                                       "<=", "<<=", ">", ">>", ">=", ">>=", "+", "++", "+=", 
                                                       "-", "--", "-=", ".", "~", "*", "*=", "/", "/=", "&", 
                                                       "&&", "&=", "^", "^", "^=", "|", "||", "|=", "!", "!=",
                                                       "%", "%=", "?", "==", "//", "...", "/*", "::" };

  Tokenizer::Tokenizer(const char* preprocessedFilename, const vector<const char*>& includePaths)
    : includePaths(includePaths)
    , buflength([]{
        int max = 0;
        for (vector<const char*>::const_iterator it = UNMERGEABLE.begin(); it != UNMERGEABLE.end(); it++) {
          int tmp = (int)strlen(*it);
          if (tmp > max) max = tmp;
        }
        return max;
      }())
    , buffer(new char[buflength]{0})
    , included(false)
    , pToken("")
    , nToken("")
    , file("")
  {
    ppFile.open(preprocessedFilename);

    for (int i = 0; i < buflength; i++) buffer[i] = ppFile.get();
  }

  Tokenizer::~Tokenizer()
  {
    ppFile.close();
    delete buffer;
  }

  string& Tokenizer::peek()
  {
    // This loop is to discard tokens when file not included
    while (pToken.length() == 0 && buffer[0] != EOF) {
      int match = 0;

      // This loop is to build the token
      while (buffer[0] != EOF) {
        if (buffer[0] == '#' && pToken.length() == 0) {
          directive();
          continue;
        } else if (buffer[0] == '/' && (buffer[1] == '*' || buffer[1] == '/')) {
          // Might not be necessary: comments are removed from preprocessed units
          comment();
          continue;
        } else if ((match = unmergeable()) > 0) {
          if (pToken.length() == 0) {
            for (int i = 0; i < match; i++) pToken += pull();
          }
          break;
        } else if (whitespace()) {
          if (pToken.length() != 0) break;
        } else if (buffer[0] == '\"' || (buffer[0] == '\'' && 
          (pToken.length() == 0 || // Catches weird unsigned long long notation
            (pToken.length() >= 2 && pToken[0] == '0' && pToken[1] == 'x')))) {
          if (pToken.length() == 0) charseq();
          break;
        } else {
          pToken += buffer[0];
        }
        pull();
      }
      if (!included)
        pToken = "";
    }

    return pToken;
  }

  string& Tokenizer::next()
  {
    if (pToken.length() == 0) peek();
    nToken = pToken;
    pToken = "";

    return nToken;
  }

  char Tokenizer::pull()
  {
    char c = buffer[0];

    for (int i = 0; i < buflength - 1; i++) {
      buffer[i] = buffer[i + 1];
    }
    buffer[buflength - 1] = ppFile.get();

    return c;
  }

  void Tokenizer::charseq()
  {
    char   terminator = buffer[0];
    int    used       = 1;

    pToken = string("") + terminator;

    for (; used < buflength && buffer[used] != terminator; used++) {
      if (buffer[used] == '\\' && ++used == buflength) pToken += ppFile.get();
      else pToken += buffer[used];
    }
    // If the buffer is completely used, means string still going
    if (used == buflength) {
      for (char c = ppFile.get(); c!= terminator && c!= EOF; c = ppFile.get()) {
        if (c == '\\') c = ppFile.get();
        pToken += c;
      }
    } else ++used;
    pToken += terminator;
    {
      int remainder = buflength - used;
      for (int i = 0; i < remainder; i++) buffer[i]             = buffer[i + used];
      for (int i = 0; i < used; i++)      buffer[i + remainder] = ppFile.get();
    }
  }

  void Tokenizer::directive()
  {
    string directive = "";
    bool   done      = false;

    for (int i = 0; i < buflength && !done; i++) {
      if (buffer[i] == '\r') continue;
      if (buffer[i] == '\n') {
        done = true;
        for (int j = 0; j < buflength; j++) buffer[j] = j < buflength - i ? buffer[i + j] : ppFile.get();
      } else directive += buffer[i];
    }
    if (!done) {
      for (char c = ppFile.get(); c != EOF && c != '\n'; c = ppFile.get()) {
        if (c == '\r') continue;
        directive += c;
      }
      for (int i = 0; i < buflength; i++) buffer[i] = ppFile.get();
    }

    {
      if (directive.length() > 4 && directive.substr(0, 5) == "#line") {
        string      folder = "";
        string      path   = "";
        const char* c_str  = directive.c_str();

        for (int i = 0; i < directive.length(); i++) {
          if (c_str[i] == '"') {
            for (++i; c_str[i] != '"'; i++) {
              if (c_str[i] == '\\') ++i;
              if (c_str[i] == SLASH) {
                path += folder + SLASH;
                folder = "";
              } else folder += c_str[i];
            }
            break;
          }
        }
        file     = path + folder;
        included = find(includePaths.begin(), includePaths.end(), path) != includePaths.end();
      }
    }
  }

  void Tokenizer::comment()
  {
    bool multi = buffer[1] == '*';

    do {
      pull();
    } while (buffer [0] != EOF && (multi ? !(buffer[0] == '*' && buffer[1] == '/') : buffer[0] != '\n'));

    pull();
    if (multi) pull();
  }

  void Tokenizer::seek(const char* token, bool checkBrackets)
  {
    int brackets[3] = { 0, 0, 0 };

    while (peek() != "" && (pToken != token || 
           (checkBrackets && (brackets[0] || brackets[1]  || brackets[2])))) {
      switch (next()[0]) {
      case '(': brackets[0]++; break;
      case ')': brackets[0]--; break;
      case '[': brackets[1]++; break;
      case ']': brackets[1]--; break;
      case '{': brackets[2]++; break;
      case '}': brackets[2]--; break;
      default: break;
      }
    }
  }

  void Tokenizer::seek(const vector<const char*>& tokens, bool checkBrackets)
  {
    int brackets[3] = { 0, 0, 0 };

    while (peek() != "" && (find(tokens.begin(), tokens.end(), pToken) == tokens.end() || 
      (checkBrackets && (brackets[0] || brackets[1]  || brackets[2])))) {
      switch (next()[0]) {
      case '(': brackets[0]++; break;
      case ')': brackets[0]--; break;
      case '[': brackets[1]++; break;
      case ']': brackets[1]--; break;
      case '{': brackets[2]++; break;
      case '}': brackets[2]--; break;
      default: break;
      }
    }
  }

  void Tokenizer::expect(const char* what)
  {
    string got = next();

    if (got != what) {
      string except = string("Token error. Expected: '") + what + "', got: '" + got + "' instead.";

      throw exception(except.c_str());
    }
  }

  string& Tokenizer::currentFile() {
    return file;
  }

  int Tokenizer::unmergeable()
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

  bool Tokenizer::whitespace()
  {
    for (int i = 0; i < WHITESPACE.size(); i++) {
      if (buffer[0] == WHITESPACE[i]) return true;
    }
    return false;
  }
}