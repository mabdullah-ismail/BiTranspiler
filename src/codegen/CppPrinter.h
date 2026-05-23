#pragma once
#include <string>
#include <sstream>
#include "../common/AST.h"

class CppPrinter : public ASTVisitor {
private:
    std::stringstream cppCode;
    int indentLevel;

    void printIndent();
    std::string getOpString(TokenType op);

public:
    CppPrinter();

    std::string getCppCode() const;

    // AST Visitor methods
    void visit(LiteralNode* node) override;
    void visit(IdentifierNode* node) override;
    void visit(BinaryOpNode* node) override;
    void visit(UnaryOpNode* node) override;
    void visit(FuncCallNode* node) override;
    void visit(BlockNode* node) override;
    void visit(VarDeclNode* node) override;
    void visit(AssignNode* node) override;
    void visit(IfNode* node) override;
    void visit(WhileNode* node) override;
    void visit(ForNode* node) override;
    void visit(ReturnNode* node) override;
    void visit(FunctionNode* node) override;
    void visit(ProgramNode* node) override;
};
