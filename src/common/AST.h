#pragma once
#include <string>
#include <vector>
#include <memory>
#include "Token.h"

// Forward declarations
class ASTVisitor;

class ASTNode {
public:
    virtual ~ASTNode() = default;
    virtual void accept(ASTVisitor* visitor) = 0;
};

class ExpressionNode : public ASTNode {
public:
    virtual ~ExpressionNode() = default;
};

class StatementNode : public ASTNode {
public:
    virtual ~StatementNode() = default;
};

// Expressions
class LiteralNode : public ExpressionNode {
public:
    std::string value;
    bool isNumeric;

    LiteralNode(std::string val, bool num) : value(val), isNumeric(num) {}
    void accept(ASTVisitor* visitor) override;
};

class IdentifierNode : public ExpressionNode {
public:
    std::string name;

    IdentifierNode(std::string n) : name(n) {}
    void accept(ASTVisitor* visitor) override;
};

class BinaryOpNode : public ExpressionNode {
public:
    std::shared_ptr<ExpressionNode> left;
    TokenType op;
    std::shared_ptr<ExpressionNode> right;

    BinaryOpNode(std::shared_ptr<ExpressionNode> l, TokenType o, std::shared_ptr<ExpressionNode> r)
        : left(l), op(o), right(r) {}
    void accept(ASTVisitor* visitor) override;
};

class UnaryOpNode : public ExpressionNode {
public:
    TokenType op;
    std::shared_ptr<ExpressionNode> operand;
    bool isPostfix; // e.g. i++ vs ++i

    UnaryOpNode(TokenType o, std::shared_ptr<ExpressionNode> opnd, bool postfix = false)
        : op(o), operand(opnd), isPostfix(postfix) {}
    void accept(ASTVisitor* visitor) override;
};

class FuncCallNode : public ExpressionNode {
public:
    std::string funcName;
    std::vector<std::shared_ptr<ExpressionNode>> args;

    FuncCallNode(std::string name, std::vector<std::shared_ptr<ExpressionNode>> a)
        : funcName(name), args(std::move(a)) {}
    void accept(ASTVisitor* visitor) override;
};

// Statements
class BlockNode : public StatementNode {
public:
    std::vector<std::shared_ptr<StatementNode>> statements;

    BlockNode() = default;
    void accept(ASTVisitor* visitor) override;
};

class VarDeclNode : public StatementNode {
public:
    std::string type;
    std::string name;
    std::shared_ptr<ExpressionNode> initVal;

    VarDeclNode(std::string t, std::string n, std::shared_ptr<ExpressionNode> init = nullptr)
        : type(t), name(n), initVal(init) {}
    void accept(ASTVisitor* visitor) override;
};

class AssignNode : public StatementNode {
public:
    std::string varName;
    std::shared_ptr<ExpressionNode> val;

    AssignNode(std::string name, std::shared_ptr<ExpressionNode> v)
        : varName(name), val(v) {}
    void accept(ASTVisitor* visitor) override;
};

class IfNode : public StatementNode {
public:
    std::shared_ptr<ExpressionNode> condition;
    std::shared_ptr<BlockNode> thenBlock;
    std::shared_ptr<BlockNode> elseBlock; // Can be nullptr

    IfNode(std::shared_ptr<ExpressionNode> cond, std::shared_ptr<BlockNode> thenB, std::shared_ptr<BlockNode> elseB = nullptr)
        : condition(cond), thenBlock(thenB), elseBlock(elseB) {}
    void accept(ASTVisitor* visitor) override;
};

class WhileNode : public StatementNode {
public:
    std::shared_ptr<ExpressionNode> condition;
    std::shared_ptr<BlockNode> body;

    WhileNode(std::shared_ptr<ExpressionNode> cond, std::shared_ptr<BlockNode> b)
        : condition(cond), body(b) {}
    void accept(ASTVisitor* visitor) override;
};

class ForNode : public StatementNode {
public:
    std::shared_ptr<StatementNode> init; // e.g. int i = 0;
    std::shared_ptr<ExpressionNode> condition;
    std::shared_ptr<ExpressionNode> update; // e.g. i++
    std::shared_ptr<BlockNode> body;

    ForNode(std::shared_ptr<StatementNode> i, std::shared_ptr<ExpressionNode> cond, std::shared_ptr<ExpressionNode> upd, std::shared_ptr<BlockNode> b)
        : init(i), condition(cond), update(upd), body(b) {}
    void accept(ASTVisitor* visitor) override;
};

class ReturnNode : public StatementNode {
public:
    std::shared_ptr<ExpressionNode> val; // Can be nullptr

    ReturnNode(std::shared_ptr<ExpressionNode> v = nullptr) : val(v) {}
    void accept(ASTVisitor* visitor) override;
};

class FunctionNode : public ASTNode {
public:
    std::string returnType;
    std::string name;
    // Parameters as pair of (type, name)
    std::vector<std::pair<std::string, std::string>> params;
    std::shared_ptr<BlockNode> body;

    FunctionNode(std::string ret, std::string n, std::vector<std::pair<std::string, std::string>> p, std::shared_ptr<BlockNode> b)
        : returnType(ret), name(n), params(std::move(p)), body(b) {}
    void accept(ASTVisitor* visitor) override;
};

class ProgramNode : public ASTNode {
public:
    std::vector<std::shared_ptr<VarDeclNode>> globals;
    std::vector<std::shared_ptr<FunctionNode>> functions;

    ProgramNode() = default;
    void accept(ASTVisitor* visitor) override;
};

// Visitor Pattern interface
class ASTVisitor {
public:
    virtual void visit(LiteralNode* node) = 0;
    virtual void visit(IdentifierNode* node) = 0;
    virtual void visit(BinaryOpNode* node) = 0;
    virtual void visit(UnaryOpNode* node) = 0;
    virtual void visit(FuncCallNode* node) = 0;
    virtual void visit(BlockNode* node) = 0;
    virtual void visit(VarDeclNode* node) = 0;
    virtual void visit(AssignNode* node) = 0;
    virtual void visit(IfNode* node) = 0;
    virtual void visit(WhileNode* node) = 0;
    virtual void visit(ForNode* node) = 0;
    virtual void visit(ReturnNode* node) = 0;
    virtual void visit(FunctionNode* node) = 0;
    virtual void visit(ProgramNode* node) = 0;
};

// Visitor accept implementations
inline void LiteralNode::accept(ASTVisitor* visitor) { visitor->visit(this); }
inline void IdentifierNode::accept(ASTVisitor* visitor) { visitor->visit(this); }
inline void BinaryOpNode::accept(ASTVisitor* visitor) { visitor->visit(this); }
inline void UnaryOpNode::accept(ASTVisitor* visitor) { visitor->visit(this); }
inline void FuncCallNode::accept(ASTVisitor* visitor) { visitor->visit(this); }
inline void BlockNode::accept(ASTVisitor* visitor) { visitor->visit(this); }
inline void VarDeclNode::accept(ASTVisitor* visitor) { visitor->visit(this); }
inline void AssignNode::accept(ASTVisitor* visitor) { visitor->visit(this); }
inline void IfNode::accept(ASTVisitor* visitor) { visitor->visit(this); }
inline void WhileNode::accept(ASTVisitor* visitor) { visitor->visit(this); }
inline void ForNode::accept(ASTVisitor* visitor) { visitor->visit(this); }
inline void ReturnNode::accept(ASTVisitor* visitor) { visitor->visit(this); }
inline void FunctionNode::accept(ASTVisitor* visitor) { visitor->visit(this); }
inline void ProgramNode::accept(ASTVisitor* visitor) { visitor->visit(this); }
