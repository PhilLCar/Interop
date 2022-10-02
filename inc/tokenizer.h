#pragma once

#include <vector>
#include <map>
#include <string>
#include <fstream>

namespace interop {
  using namespace std;

  class Tokenizer {
  public:
     Tokenizer(const char* preprocessedFilename, const vector<const char*>& includePaths);
     ~Tokenizer();

  public:
    string& peek();
    string& next();
    void    seek(const char* token, bool checkBrackets = true);
    void    seek(const vector<const char*>& tokens, bool checkBrackets = true);
    void    expect(const char *what);

    string& currentFile();

  private:
    char   pull();
    void   charseq();
    void   directive();
    void   comment();

    int    unmergeable();
    bool   whitespace();

  private:
    static const vector<char>                    WHITESPACE;
    static const vector<const char*>             PUNCTUATION;
  public:
    static const map<const string, const string> OPERATORS;

  private:
    const vector<const char*>& includePaths;
    const int                  buflength;

  private:
    ifstream       ppFile;
    string         pToken;
    string         nToken;
    char*          buffer;
    bool           included;
    string         file;
  };
}