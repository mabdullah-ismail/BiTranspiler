#pragma once
#include <vector>
#include <memory>
#include "../common/Token.h"
#include "../common/AST.h"

class CppParser {
private:
    std::vector<Token> tokens;
    size_t current;

    Token peek() const;
    Token peekNext() const;
    Token previous() const;
    bool isAtEnd() const;
    Token advance();
    bool check(TokenType type) const;
    bool match(const std::vector<TokenType>& types);
    Token consume(TokenType type, const std::string& message);

    // Parsing expressions (Recursive Descent with precedence)
    std::shared_ptr<ExpressionNode> parseExpression();
    std::shared_ptr<ExpressionNode> parseAssignment();
    std::shared_ptr<ExpressionNode> parseLogicalOr();
    std::shared_ptr<ExpressionNode> parseLogicalAnd();
    std::shared_ptr<ExpressionNode> parseBitwiseOr();
    std::shared_ptr<ExpressionNode> parseBitwiseXor();
    std::shared_ptr<ExpressionNode> parseBitwiseAnd();
    std::shared_ptr<ExpressionNode> parseEquality();
    std::shared_ptr<ExpressionNode> parseComparison();
    std::shared_ptr<ExpressionNode> parseShift();
    std::shared_ptr<ExpressionNode> parseAdditive();
    std::shared_ptr<ExpressionNode> parseMultiplicative();
    std::shared_ptr<ExpressionNode> parseUnary();
    std::shared_ptr<ExpressionNode> parsePrimary();

    // Parsing statements
    std::shared_ptr<StatementNode> parseStatement();
    std::shared_ptr<StatementNode> parseDeclarationStatement();
    std::shared_ptr<StatementNode> parseExpressionStatement();
    std::shared_ptr<BlockNode> parseBlockStatement();
    std::shared_ptr<StatementNode> parseIfStatement();
    std::shared_ptr<StatementNode> parseWhileStatement();
    std::shared_ptr<StatementNode> parseForStatement();
    std::shared_ptr<StatementNode> parseReturnStatement();

    // High level units
    std::shared_ptr<FunctionNode> parseFunction(const std::string& returnType, const std::string& name);

public:
    explicit CppParser(std::vector<Token> tkns);
    std::shared_ptr<ProgramNode> parse();
};
