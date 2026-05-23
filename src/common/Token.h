#pragma once
#include <string>
#include <iostream>

enum class TokenType {
    // Shared / Common
    UNKNOWN,
    EOF_TOKEN,
    IDENTIFIER,
    NUMBER,
    STRING,
    COMMENT,

    // C++ Keywords
    KEYWORD_INT,
    KEYWORD_CHAR,
    KEYWORD_VOID,
    KEYWORD_IF,
    KEYWORD_ELSE,
    KEYWORD_WHILE,
    KEYWORD_FOR,
    KEYWORD_RETURN,

    // C++ Operators & Punctuation
    OP_ASSIGN,      // =
    OP_PLUS,        // +
    OP_MINUS,       // -
    OP_MUL,         // *
    OP_DIV,         // /
    OP_INC,         // ++
    OP_DEC,         // --
    OP_EQ,          // ==
    OP_NE,          // !=
    OP_LT,          // <
    OP_GT,          // >
    OP_LE,          // <=
    OP_GE,          // >=
    OP_BIT_AND,     // &
    OP_BIT_OR,      // |
    OP_BIT_XOR,     // ^
    OP_BIT_NOT,     // ~
    OP_LSHIFT,      // <<
    OP_RSHIFT,      // >>
    OP_LOG_AND,     // &&
    OP_LOG_OR,      // ||
    OP_LOG_NOT,     // !
    
    LPAREN,         // (
    RPAREN,         // )
    LBRACE,         // {
    RBRACE,         // }
    LBRACKET,       // [
    RBRACKET,       // ]
    SEMICOLON,      // ;
    COMMA,          // ,

    // MASM Instructions
    INST_MOV, INST_PUSH, INST_POP, INST_XCHG,
    INST_ADD, INST_SUB, INST_MUL, INST_DIV, INST_INC, INST_DEC, INST_NEG,
    INST_AND, INST_OR, INST_XOR, INST_NOT, INST_SHL, INST_SHR,
    INST_CMP, INST_JE, INST_JNE, INST_JG, INST_JL, INST_JGE, INST_JLE, INST_JMP,
    INST_CALL, INST_RET, INST_LOOP,
    INST_SETE, INST_SETNE, INST_SETG, INST_SETL, INST_SETGE, INST_SETLE, INST_MOVZX,

    // MASM Directives
    DIR_DATA, DIR_CODE, DIR_PROC, DIR_ENDP, DIR_DB, DIR_DW, DIR_DD, DIR_EQU,
    DIR_MODEL, DIR_386, DIR_STACK,

    // MASM Registers (32-bit, 16-bit, 8-bit low, 8-bit high)
    REG_EAX, REG_EBX, REG_ECX, REG_EDX, REG_ESI, REG_EDI, REG_ESP, REG_EBP,
    REG_AX, REG_BX, REG_CX, REG_DX,
    REG_AL, REG_BL, REG_CL, REG_DL,
    REG_AH, REG_BH, REG_CH, REG_DH,

    COLON // :
};

struct Token {
    TokenType type;
    std::string value;
    int line;
    int column;

    Token(TokenType t, std::string val, int l, int c)
        : type(t), value(std::move(val)), line(l), column(c) {}
};

inline std::string tokenTypeToString(TokenType type) {
    switch (type) {
        case TokenType::UNKNOWN: return "UNKNOWN";
        case TokenType::EOF_TOKEN: return "EOF";
        case TokenType::IDENTIFIER: return "IDENTIFIER";
        case TokenType::NUMBER: return "NUMBER";
        case TokenType::STRING: return "STRING";
        case TokenType::COMMENT: return "COMMENT";
        case TokenType::KEYWORD_INT: return "int";
        case TokenType::KEYWORD_CHAR: return "char";
        case TokenType::KEYWORD_VOID: return "void";
        case TokenType::KEYWORD_IF: return "if";
        case TokenType::KEYWORD_ELSE: return "else";
        case TokenType::KEYWORD_WHILE: return "while";
        case TokenType::KEYWORD_FOR: return "for";
        case TokenType::KEYWORD_RETURN: return "return";
        case TokenType::OP_ASSIGN: return "=";
        case TokenType::OP_PLUS: return "+";
        case TokenType::OP_MINUS: return "-";
        case TokenType::OP_MUL: return "*";
        case TokenType::OP_DIV: return "/";
        case TokenType::OP_INC: return "++";
        case TokenType::OP_DEC: return "--";
        case TokenType::OP_EQ: return "==";
        case TokenType::OP_NE: return "!=";
        case TokenType::OP_LT: return "<";
        case TokenType::OP_GT: return ">";
        case TokenType::OP_LE: return "<=";
        case TokenType::OP_GE: return ">=";
        case TokenType::OP_BIT_AND: return "&";
        case TokenType::OP_BIT_OR: return "|";
        case TokenType::OP_BIT_XOR: return "^";
        case TokenType::OP_BIT_NOT: return "~";
        case TokenType::OP_LSHIFT: return "<<";
        case TokenType::OP_RSHIFT: return ">>";
        case TokenType::OP_LOG_AND: return "&&";
        case TokenType::OP_LOG_OR: return "||";
        case TokenType::OP_LOG_NOT: return "!";
        case TokenType::LPAREN: return "(";
        case TokenType::RPAREN: return ")";
        case TokenType::LBRACE: return "{";
        case TokenType::RBRACE: return "}";
        case TokenType::LBRACKET: return "[";
        case TokenType::RBRACKET: return "]";
        case TokenType::SEMICOLON: return ";";
        case TokenType::COMMA: return ",";
        case TokenType::INST_MOV: return "MOV";
        case TokenType::INST_PUSH: return "PUSH";
        case TokenType::INST_POP: return "POP";
        case TokenType::INST_XCHG: return "XCHG";
        case TokenType::INST_ADD: return "ADD";
        case TokenType::INST_SUB: return "SUB";
        case TokenType::INST_MUL: return "MUL";
        case TokenType::INST_DIV: return "DIV";
        case TokenType::INST_INC: return "INC";
        case TokenType::INST_DEC: return "DEC";
        case TokenType::INST_NEG: return "NEG";
        case TokenType::INST_AND: return "AND";
        case TokenType::INST_OR: return "OR";
        case TokenType::INST_XOR: return "XOR";
        case TokenType::INST_NOT: return "NOT";
        case TokenType::INST_SHL: return "SHL";
        case TokenType::INST_SHR: return "SHR";
        case TokenType::INST_CMP: return "CMP";
        case TokenType::INST_JE: return "JE";
        case TokenType::INST_JNE: return "JNE";
        case TokenType::INST_JG: return "JG";
        case TokenType::INST_JL: return "JL";
        case TokenType::INST_JGE: return "JGE";
        case TokenType::INST_JLE: return "JLE";
        case TokenType::INST_JMP: return "JMP";
        case TokenType::INST_CALL: return "CALL";
        case TokenType::INST_RET: return "RET";
        case TokenType::INST_LOOP: return "LOOP";
        case TokenType::INST_SETE: return "SETE";
        case TokenType::INST_SETNE: return "SETNE";
        case TokenType::INST_SETG: return "SETG";
        case TokenType::INST_SETL: return "SETL";
        case TokenType::INST_SETGE: return "SETGE";
        case TokenType::INST_SETLE: return "SETLE";
        case TokenType::INST_MOVZX: return "MOVZX";
        case TokenType::DIR_DATA: return ".data";
        case TokenType::DIR_CODE: return ".code";
        case TokenType::DIR_PROC: return "PROC";
        case TokenType::DIR_ENDP: return "ENDP";
        case TokenType::DIR_DB: return "DB";
        case TokenType::DIR_DW: return "DW";
        case TokenType::DIR_DD: return "DD";
        case TokenType::DIR_EQU: return "EQU";
        case TokenType::DIR_MODEL: return ".model";
        case TokenType::DIR_386: return ".386";
        case TokenType::DIR_STACK: return ".stack";
        case TokenType::REG_EAX: return "EAX";
        case TokenType::REG_EBX: return "EBX";
        case TokenType::REG_ECX: return "ECX";
        case TokenType::REG_EDX: return "EDX";
        case TokenType::REG_ESI: return "ESI";
        case TokenType::REG_EDI: return "EDI";
        case TokenType::REG_ESP: return "ESP";
        case TokenType::REG_EBP: return "EBP";
        case TokenType::REG_AX: return "AX";
        case TokenType::REG_BX: return "BX";
        case TokenType::REG_CX: return "CX";
        case TokenType::REG_DX: return "DX";
        case TokenType::REG_AL: return "AL";
        case TokenType::REG_BL: return "BL";
        case TokenType::REG_CL: return "CL";
        case TokenType::REG_DL: return "DL";
        case TokenType::REG_AH: return "AH";
        case TokenType::REG_BH: return "BH";
        case TokenType::REG_CH: return "CH";
        case TokenType::REG_DH: return "DH";
        case TokenType::COLON: return ":";
        default: return "UNKNOWN";
    }
}
