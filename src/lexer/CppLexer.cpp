#include "CppLexer.h"
#include <cctype>
#include <map>

CppLexer::CppLexer(std::string src)
    : source(std::move(src)), position(0), line(1), column(1) {}

char CppLexer::peek() const {
    if (position >= source.length()) {
        return '\0';
    }
    return source[position];
}

char CppLexer::peekNext() const {
    if (position + 1 >= source.length()) {
        return '\0';
    }
    return source[position + 1];
}

char CppLexer::get() {
    if (position >= source.length()) {
        return '\0';
    }
    char c = source[position++];
    if (c == '\n') {
        line++;
        column = 1;
    } else {
        column++;
    }
    return c;
}

void CppLexer::skipWhitespace() {
    while (true) {
        char c = peek();
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            get();
        } else {
            break;
        }
    }
}

void CppLexer::skipComment() {
    // Check for single line comments //
    if (peek() == '/' && peekNext() == '/') {
        get(); // consume '/'
        get(); // consume '/'
        while (peek() != '\n' && peek() != '\0') {
            get();
        }
    }
    // Check for multi-line comments /* */
    else if (peek() == '/' && peekNext() == '*') {
        get(); // consume '/'
        get(); // consume '*'
        while (peek() != '\0') {
            if (peek() == '*' && peekNext() == '/') {
                get(); // consume '*'
                get(); // consume '/'
                break;
            }
            get();
        }
    }
}

Token CppLexer::readIdentifierOrKeyword() {
    int startCol = column;
    std::string val;
    while (true) {
        char c = peek();
        if (std::isalnum(c) || c == '_') {
            val += get();
        } else {
            break;
        }
    }

    static const std::map<std::string, TokenType> keywords = {
        {"int", TokenType::KEYWORD_INT},
        {"char", TokenType::KEYWORD_CHAR},
        {"void", TokenType::KEYWORD_VOID},
        {"if", TokenType::KEYWORD_IF},
        {"else", TokenType::KEYWORD_ELSE},
        {"while", TokenType::KEYWORD_WHILE},
        {"for", TokenType::KEYWORD_FOR},
        {"return", TokenType::KEYWORD_RETURN}
    };

    auto it = keywords.find(val);
    if (it != keywords.end()) {
        return Token(it->second, val, line, startCol);
    }
    return Token(TokenType::IDENTIFIER, val, line, startCol);
}

Token CppLexer::readNumber() {
    int startCol = column;
    std::string val;
    while (std::isdigit(peek())) {
        val += get();
    }
    return Token(TokenType::NUMBER, val, line, startCol);
}

Token CppLexer::readString() {
    int startCol = column;
    get(); // consume starting quote (double or single)
    std::string val;
    // We support simple string literals or character literals without escape sequence parsing for now
    while (peek() != '"' && peek() != '\'' && peek() != '\0') {
        val += get();
    }
    if (peek() != '\0') {
        get(); // consume ending quote
    }
    return Token(TokenType::STRING, val, line, startCol);
}

std::vector<Token> CppLexer::tokenize() {
    std::vector<Token> tokens;

    while (position < source.length()) {
        skipWhitespace();
        char c = peek();
        if (c == '\0') break;

        // Comments
        if (c == '/' && (peekNext() == '/' || peekNext() == '*')) {
            skipComment();
            continue;
        }

        int startCol = column;

        // Strings & characters
        if (c == '"' || c == '\'') {
            tokens.push_back(readString());
            continue;
        }

        // Identifiers & Keywords
        if (std::isalpha(c) || c == '_') {
            tokens.push_back(readIdentifierOrKeyword());
            continue;
        }

        // Numbers
        if (std::isdigit(c)) {
            tokens.push_back(readNumber());
            continue;
        }

        // Operators & Punctuation
        get(); // consume c
        std::string s(1, c);

        switch (c) {
            case '+':
                if (peek() == '+') { get(); tokens.emplace_back(TokenType::OP_INC, "++", line, startCol); }
                else { tokens.emplace_back(TokenType::OP_PLUS, "+", line, startCol); }
                break;
            case '-':
                if (peek() == '-') { get(); tokens.emplace_back(TokenType::OP_DEC, "--", line, startCol); }
                else { tokens.emplace_back(TokenType::OP_MINUS, "-", line, startCol); }
                break;
            case '*':
                tokens.emplace_back(TokenType::OP_MUL, "*", line, startCol);
                break;
            case '/':
                tokens.emplace_back(TokenType::OP_DIV, "/", line, startCol);
                break;
            case '=':
                if (peek() == '=') { get(); tokens.emplace_back(TokenType::OP_EQ, "==", line, startCol); }
                else { tokens.emplace_back(TokenType::OP_ASSIGN, "=", line, startCol); }
                break;
            case '!':
                if (peek() == '=') { get(); tokens.emplace_back(TokenType::OP_NE, "!=", line, startCol); }
                else { tokens.emplace_back(TokenType::OP_LOG_NOT, "!", line, startCol); }
                break;
            case '<':
                if (peek() == '=') { get(); tokens.emplace_back(TokenType::OP_LE, "<=", line, startCol); }
                else if (peek() == '<') { get(); tokens.emplace_back(TokenType::OP_LSHIFT, "<<", line, startCol); }
                else { tokens.emplace_back(TokenType::OP_LT, "<", line, startCol); }
                break;
            case '>':
                if (peek() == '=') { get(); tokens.emplace_back(TokenType::OP_GE, ">=", line, startCol); }
                else if (peek() == '>') { get(); tokens.emplace_back(TokenType::OP_RSHIFT, ">>", line, startCol); }
                else { tokens.emplace_back(TokenType::OP_GT, ">", line, startCol); }
                break;
            case '&':
                if (peek() == '&') { get(); tokens.emplace_back(TokenType::OP_LOG_AND, "&&", line, startCol); }
                else { tokens.emplace_back(TokenType::OP_BIT_AND, "&", line, startCol); }
                break;
            case '|':
                if (peek() == '|') { get(); tokens.emplace_back(TokenType::OP_LOG_OR, "||", line, startCol); }
                else { tokens.emplace_back(TokenType::OP_BIT_OR, "|", line, startCol); }
                break;
            case '^':
                tokens.emplace_back(TokenType::OP_BIT_XOR, "^", line, startCol);
                break;
            case '~':
                tokens.emplace_back(TokenType::OP_BIT_NOT, "~", line, startCol);
                break;
            case '(':
                tokens.emplace_back(TokenType::LPAREN, "(", line, startCol);
                break;
            case ')':
                tokens.emplace_back(TokenType::RPAREN, ")", line, startCol);
                break;
            case '{':
                tokens.emplace_back(TokenType::LBRACE, "{", line, startCol);
                break;
            case '}':
                tokens.emplace_back(TokenType::RBRACE, "}", line, startCol);
                break;
            case '[':
                tokens.emplace_back(TokenType::LBRACKET, "[", line, startCol);
                break;
            case ']':
                tokens.emplace_back(TokenType::RBRACKET, "]", line, startCol);
                break;
            case ';':
                tokens.emplace_back(TokenType::SEMICOLON, ";", line, startCol);
                break;
            case ',':
                tokens.emplace_back(TokenType::COMMA, ",", line, startCol);
                break;
            default:
                tokens.emplace_back(TokenType::UNKNOWN, s, line, startCol);
                break;
        }
    }

    tokens.emplace_back(TokenType::EOF_TOKEN, "", line, column);
    return tokens;
}
