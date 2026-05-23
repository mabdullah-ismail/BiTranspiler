#pragma once
#include <string>
#include <sstream>
#include <vector>
#include "../common/AST.h"

class AsmPrinter : public ASTVisitor {
private:
    std::stringstream asmCode;
    int labelCounter;
    std::string currentFunction;
    bool inExpression;

    // Helper to generate unique labels
    std::string generateLabel(const std::string& prefix);

public:
    AsmPrinter();

    std::string getAsmCode() const;

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
