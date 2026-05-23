#include "AsmLexer.h"
#include <cctype>
#include <map>
#include <algorithm>

AsmLexer::AsmLexer(std::string src)
    : source(std::move(src)), position(0), line(1), column(1) {}

char AsmLexer::peek() const {
    if (position >= source.length()) {
        return '\0';
    }
    return source[position];
}

char AsmLexer::peekNext() const {
    if (position + 1 >= source.length()) {
        return '\0';
    }
    return source[position + 1];
}

char AsmLexer::get() {
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

void AsmLexer::skipWhitespace() {
    while (true) {
        char c = peek();
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            get();
        } else {
            break;
        }
    }
}

void AsmLexer::skipComment() {
    if (peek() == ';') {
        while (peek() != '\n' && peek() != '\0') {
            get();
        }
    }
}

Token AsmLexer::readIdentifierOrKeyword() {
    int startCol = column;
    std::string val;
    
    // First character can be letter, _, ., or @
    val += get();
    
    while (true) {
        char c = peek();
        if (std::isalnum(c) || c == '_' || c == '?' || c == '@' || c == '.') {
            val += get();
        } else {
            break;
        }
    }

    // Convert value to uppercase for matching
    std::string upperVal = val;
    std::transform(upperVal.begin(), upperVal.end(), upperVal.begin(), [](unsigned char c) {
        return std::toupper(c);
    });

    static const std::map<std::string, TokenType> keywords = {
        // Instructions
        {"MOV", TokenType::INST_MOV},
        {"PUSH", TokenType::INST_PUSH},
        {"POP", TokenType::INST_POP},
        {"XCHG", TokenType::INST_XCHG},
        {"ADD", TokenType::INST_ADD},
        {"SUB", TokenType::INST_SUB},
        {"MUL", TokenType::INST_MUL},
        {"IMUL", TokenType::INST_MUL},
        {"DIV", TokenType::INST_DIV},
        {"IDIV", TokenType::INST_DIV},
        {"INC", TokenType::INST_INC},
        {"DEC", TokenType::INST_DEC},
        {"NEG", TokenType::INST_NEG},
        {"AND", TokenType::INST_AND},
        {"OR", TokenType::INST_OR},
        {"XOR", TokenType::INST_XOR},
        {"NOT", TokenType::INST_NOT},
        {"SHL", TokenType::INST_SHL},
        {"SHR", TokenType::INST_SHR},
        {"CMP", TokenType::INST_CMP},
        {"JE", TokenType::INST_JE},
        {"JNE", TokenType::INST_JNE},
        {"JG", TokenType::INST_JG},
        {"JL", TokenType::INST_JL},
        {"JGE", TokenType::INST_JGE},
        {"JLE", TokenType::INST_JLE},
        {"JMP", TokenType::INST_JMP},
        {"CALL", TokenType::INST_CALL},
        {"RET", TokenType::INST_RET},
        {"LOOP", TokenType::INST_LOOP},
        {"SETE", TokenType::INST_SETE},
        {"SETNE", TokenType::INST_SETNE},
        {"SETG", TokenType::INST_SETG},
        {"SETL", TokenType::INST_SETL},
        {"SETGE", TokenType::INST_SETGE},
        {"SETLE", TokenType::INST_SETLE},
        {"MOVZX", TokenType::INST_MOVZX},

        // Directives
        {".DATA", TokenType::DIR_DATA},
        {"DATA", TokenType::DIR_DATA},
        {".CODE", TokenType::DIR_CODE},
        {"CODE", TokenType::DIR_CODE},
        {"PROC", TokenType::DIR_PROC},
        {"ENDP", TokenType::DIR_ENDP},
        {"DB", TokenType::DIR_DB},
        {"DW", TokenType::DIR_DW},
        {"DD", TokenType::DIR_DD},
        {"EQU", TokenType::DIR_EQU},
        {".MODEL", TokenType::DIR_MODEL},
        {".386", TokenType::DIR_386},
        {".STACK", TokenType::DIR_STACK},

        // Registers
        {"EAX", TokenType::REG_EAX},
        {"EBX", TokenType::REG_EBX},
        {"ECX", TokenType::REG_ECX},
        {"EDX", TokenType::REG_EDX},
        {"ESI", TokenType::REG_ESI},
        {"EDI", TokenType::REG_EDI},
        {"ESP", TokenType::REG_ESP},
        {"EBP", TokenType::REG_EBP},
        {"AX", TokenType::REG_AX},
        {"BX", TokenType::REG_BX},
        {"CX", TokenType::REG_CX},
        {"DX", TokenType::REG_DX},
        {"AL", TokenType::REG_AL},
        {"BL", TokenType::REG_BL},
        {"CL", TokenType::REG_CL},
        {"DL", TokenType::REG_DL},
        {"AH", TokenType::REG_AH},
        {"BH", TokenType::REG_BH},
        {"CH", TokenType::REG_CH},
        {"DH", TokenType::REG_DH}
    };

    auto it = keywords.find(upperVal);
    if (it != keywords.end()) {
        return Token(it->second, val, line, startCol);
    }
    return Token(TokenType::IDENTIFIER, val, line, startCol);
}

Token AsmLexer::readNumber() {
    int startCol = column;
    std::string val;
    
    // Read alphanumeric characters to capture suffixes like 'h' or 'b'
    while (std::isalnum(peek())) {
        val += get();
    }
    
    // It's a number token. The value can contain suffixes.
    return Token(TokenType::NUMBER, val, line, startCol);
}

Token AsmLexer::readString() {
    int startCol = column;
    char quoteChar = get(); // consume quote char (' or ")
    std::string val;
    while (peek() != quoteChar && peek() != '\0') {
        val += get();
    }
    if (peek() != '\0') {
        get(); // consume ending quote
    }
    return Token(TokenType::STRING, val, line, startCol);
}

std::vector<Token> AsmLexer::tokenize() {
    std::vector<Token> tokens;

    while (position < source.length()) {
        skipWhitespace();
        char c = peek();
        if (c == '\0') break;

        // Comments
        if (c == ';') {
            skipComment();
            continue;
        }

        int startCol = column;

        // Strings & characters
        if (c == '"' || c == '\'') {
            tokens.push_back(readString());
            continue;
        }

        // Identifiers (including registers, directives, labels)
        // MASM identifiers can start with a letter, _, ., or @
        if (std::isalpha(c) || c == '_' || c == '.' || c == '@') {
            tokens.push_back(readIdentifierOrKeyword());
            continue;
        }

        // Numbers (MASM numbers can be e.g. 10, 0Ah, 1010b. Note that hex numbers
        // must start with a digit: if they start with a letter they are treated as identifiers,
        // so hex 0FFFFh starts with 0).
        if (std::isdigit(c)) {
            tokens.push_back(readNumber());
            continue;
        }

        // Punctuation & Operators
        get(); // consume c
        std::string s(1, c);

        switch (c) {
            case ',':
                tokens.emplace_back(TokenType::COMMA, ",", line, startCol);
                break;
            case ':':
                tokens.emplace_back(TokenType::COLON, ":", line, startCol);
                break;
            case '[':
                tokens.emplace_back(TokenType::LBRACKET, "[", line, startCol);
                break;
            case ']':
                tokens.emplace_back(TokenType::RBRACKET, "]", line, startCol);
                break;
            case '+':
                tokens.emplace_back(TokenType::OP_PLUS, "+", line, startCol);
                break;
            case '-':
                tokens.emplace_back(TokenType::OP_MINUS, "-", line, startCol);
                break;
            case '*':
                tokens.emplace_back(TokenType::OP_MUL, "*", line, startCol);
                break;
            case '/':
                tokens.emplace_back(TokenType::OP_DIV, "/", line, startCol);
                break;
            default:
                tokens.emplace_back(TokenType::UNKNOWN, s, line, startCol);
                break;
        }
    }

    tokens.emplace_back(TokenType::EOF_TOKEN, "", line, column);
    return tokens;
}
