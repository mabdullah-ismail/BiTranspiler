#pragma once
#include <string>
#include <vector>
#include "../common/Token.h"

class AsmLexer {
private:
    std::string source;
    size_t position;
    int line;
    int column;

    char peek() const;
    char peekNext() const;
    char get();
    void skipWhitespace();
    void skipComment();
    Token readIdentifierOrKeyword();
    Token readNumber();
    Token readString();

public:
    explicit AsmLexer(std::string src);
    std::vector<Token> tokenize();
};
