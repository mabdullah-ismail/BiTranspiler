#pragma once
#include <vector>
#include <memory>
#include <string>
#include "../common/Token.h"
#include "../common/AST.h"

struct AsmInstruction {
    TokenType op;
    std::vector<std::string> args;
    std::string label;
    std::string comment;
    int line;

    AsmInstruction() : op(TokenType::UNKNOWN), label(""), comment(""), line(0) {}
};

class AsmParser {
private:
    std::vector<Token> tokens;
    size_t current;
    std::vector<std::string> warnings;

    Token peek() const;
    Token peekNext() const;
    Token previous() const;
    bool isAtEnd() const;
    Token advance();
    bool check(TokenType type) const;
    bool match(const std::vector<TokenType>& types);
    Token consume(TokenType type, const std::string& message);
    void skipNewlines();

    std::shared_ptr<ExpressionNode> makeExprFromArg(const std::string& arg);
    std::shared_ptr<ExpressionNode> recoverCondition(TokenType jumpOp, const std::string& arg1, const std::string& arg2);

    std::shared_ptr<BlockNode> recoverControlFlow(const std::vector<AsmInstruction>& insts, int start, int end);

public:
    explicit AsmParser(std::vector<Token> tkns);
    std::shared_ptr<ProgramNode> parse();
    std::vector<std::string> getWarnings() const;
};
