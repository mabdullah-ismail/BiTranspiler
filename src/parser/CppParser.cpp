#include "CppParser.h"
#include <stdexcept>
#include <iostream>

CppParser::CppParser(std::vector<Token> tkns) : tokens(std::move(tkns)), current(0) {}

Token CppParser::peek() const {
    if (isAtEnd()) return tokens.back();
    return tokens[current];
}

Token CppParser::peekNext() const {
    if (current + 1 >= tokens.size()) return tokens.back();
    return tokens[current + 1];
}

Token CppParser::previous() const {
    if (current == 0) return tokens.front();
    return tokens[current - 1];
}

bool CppParser::isAtEnd() const {
    return current >= tokens.size() || tokens[current].type == TokenType::EOF_TOKEN;
}

Token CppParser::advance() {
    if (!isAtEnd()) current++;
    return previous();
}

bool CppParser::check(TokenType type) const {
    if (isAtEnd()) return false;
    return peek().type == type;
}

bool CppParser::match(const std::vector<TokenType>& types) {
    for (TokenType type : types) {
        if (check(type)) {
            advance();
            return true;
        }
    }
    return false;
}

Token CppParser::consume(TokenType type, const std::string& message) {
    if (check(type)) return advance();
    throw std::runtime_error("Parser Error: " + message + " at line " + std::to_string(peek().line) + ", col " + std::to_string(peek().column));
}

std::shared_ptr<ExpressionNode> CppParser::parseExpression() {
    return parseAssignment();
}

std::shared_ptr<ExpressionNode> CppParser::parseAssignment() {
    std::shared_ptr<ExpressionNode> expr = parseLogicalOr();

    if (match({TokenType::OP_ASSIGN})) {
        Token op = previous();
        std::shared_ptr<ExpressionNode> value = parseAssignment();

        // Check if left hand side is an identifier
        auto idExpr = std::dynamic_pointer_cast<IdentifierNode>(expr);
        if (idExpr) {
            // Re-interpret as assignment statement or return an assignment node
            // Since Assignment can be an expression in C++, we can keep it as BinaryOpNode or Assignment
            return std::make_shared<BinaryOpNode>(expr, op.type, value);
        }
        throw std::runtime_error("Invalid assignment target at line " + std::to_string(op.line));
    }

    return expr;
}

std::shared_ptr<ExpressionNode> CppParser::parseLogicalOr() {
    std::shared_ptr<ExpressionNode> expr = parseLogicalAnd();
    while (match({TokenType::OP_LOG_OR})) {
        TokenType op = previous().type;
        std::shared_ptr<ExpressionNode> right = parseLogicalAnd();
        expr = std::make_shared<BinaryOpNode>(expr, op, right);
    }
    return expr;
}

std::shared_ptr<ExpressionNode> CppParser::parseLogicalAnd() {
    std::shared_ptr<ExpressionNode> expr = parseBitwiseOr();
    while (match({TokenType::OP_LOG_AND})) {
        TokenType op = previous().type;
        std::shared_ptr<ExpressionNode> right = parseBitwiseOr();
        expr = std::make_shared<BinaryOpNode>(expr, op, right);
    }
    return expr;
}

std::shared_ptr<ExpressionNode> CppParser::parseBitwiseOr() {
    std::shared_ptr<ExpressionNode> expr = parseBitwiseXor();
    while (match({TokenType::OP_BIT_OR})) {
        TokenType op = previous().type;
        std::shared_ptr<ExpressionNode> right = parseBitwiseXor();
        expr = std::make_shared<BinaryOpNode>(expr, op, right);
    }
    return expr;
}

std::shared_ptr<ExpressionNode> CppParser::parseBitwiseXor() {
    std::shared_ptr<ExpressionNode> expr = parseBitwiseAnd();
    while (match({TokenType::OP_BIT_XOR})) {
        TokenType op = previous().type;
        std::shared_ptr<ExpressionNode> right = parseBitwiseAnd();
        expr = std::make_shared<BinaryOpNode>(expr, op, right);
    }
    return expr;
}

std::shared_ptr<ExpressionNode> CppParser::parseBitwiseAnd() {
    std::shared_ptr<ExpressionNode> expr = parseEquality();
    while (match({TokenType::OP_BIT_AND})) {
        TokenType op = previous().type;
        std::shared_ptr<ExpressionNode> right = parseEquality();
        expr = std::make_shared<BinaryOpNode>(expr, op, right);
    }
    return expr;
}

std::shared_ptr<ExpressionNode> CppParser::parseEquality() {
    std::shared_ptr<ExpressionNode> expr = parseComparison();
    while (match({TokenType::OP_EQ, TokenType::OP_NE})) {
        TokenType op = previous().type;
        std::shared_ptr<ExpressionNode> right = parseComparison();
        expr = std::make_shared<BinaryOpNode>(expr, op, right);
    }
    return expr;
}

std::shared_ptr<ExpressionNode> CppParser::parseComparison() {
    std::shared_ptr<ExpressionNode> expr = parseShift();
    while (match({TokenType::OP_LT, TokenType::OP_GT, TokenType::OP_LE, TokenType::OP_GE})) {
        TokenType op = previous().type;
        std::shared_ptr<ExpressionNode> right = parseShift();
        expr = std::make_shared<BinaryOpNode>(expr, op, right);
    }
    return expr;
}

std::shared_ptr<ExpressionNode> CppParser::parseShift() {
    std::shared_ptr<ExpressionNode> expr = parseAdditive();
    while (match({TokenType::OP_LSHIFT, TokenType::OP_RSHIFT})) {
        TokenType op = previous().type;
        std::shared_ptr<ExpressionNode> right = parseAdditive();
        expr = std::make_shared<BinaryOpNode>(expr, op, right);
    }
    return expr;
}

std::shared_ptr<ExpressionNode> CppParser::parseAdditive() {
    std::shared_ptr<ExpressionNode> expr = parseMultiplicative();
    while (match({TokenType::OP_PLUS, TokenType::OP_MINUS})) {
        TokenType op = previous().type;
        std::shared_ptr<ExpressionNode> right = parseMultiplicative();
        expr = std::make_shared<BinaryOpNode>(expr, op, right);
    }
    return expr;
}

std::shared_ptr<ExpressionNode> CppParser::parseMultiplicative() {
    std::shared_ptr<ExpressionNode> expr = parseUnary();
    while (match({TokenType::OP_MUL, TokenType::OP_DIV})) {
        TokenType op = previous().type;
        std::shared_ptr<ExpressionNode> right = parseUnary();
        expr = std::make_shared<BinaryOpNode>(expr, op, right);
    }
    return expr;
}

std::shared_ptr<ExpressionNode> CppParser::parseUnary() {
    if (match({TokenType::OP_MINUS, TokenType::OP_LOG_NOT, TokenType::OP_BIT_NOT, TokenType::OP_INC, TokenType::OP_DEC})) {
        TokenType op = previous().type;
        std::shared_ptr<ExpressionNode> operand = parseUnary();
        return std::make_shared<UnaryOpNode>(op, operand, false);
    }

    std::shared_ptr<ExpressionNode> expr = parsePrimary();

    // Check for postfix operators like i++ or i--
    if (match({TokenType::OP_INC, TokenType::OP_DEC})) {
        TokenType op = previous().type;
        return std::make_shared<UnaryOpNode>(op, expr, true);
    }

    return expr;
}

std::shared_ptr<ExpressionNode> CppParser::parsePrimary() {
    if (match({TokenType::NUMBER})) {
        return std::make_shared<LiteralNode>(previous().value, true);
    }
    if (match({TokenType::STRING})) {
        return std::make_shared<LiteralNode>(previous().value, false);
    }
    if (match({TokenType::IDENTIFIER})) {
        std::string name = previous().value;
        // Function call check
        if (match({TokenType::LPAREN})) {
            std::vector<std::shared_ptr<ExpressionNode>> args;
            if (!check(TokenType::RPAREN)) {
                do {
                    args.push_back(parseExpression());
                } while (match({TokenType::COMMA}));
            }
            consume(TokenType::RPAREN, "Expect ')' after arguments");
            return std::make_shared<FuncCallNode>(name, args);
        }
        return std::make_shared<IdentifierNode>(name);
    }
    if (match({TokenType::LPAREN})) {
        std::shared_ptr<ExpressionNode> expr = parseExpression();
        consume(TokenType::RPAREN, "Expect ')' after expression");
        return expr;
    }

    throw std::runtime_error("Expect expression at line " + std::to_string(peek().line) + ", column " + std::to_string(peek().column) + ". Found: " + peek().value);
}

std::shared_ptr<StatementNode> CppParser::parseStatement() {
    if (match({TokenType::KEYWORD_IF})) return parseIfStatement();
    if (match({TokenType::KEYWORD_WHILE})) return parseWhileStatement();
    if (match({TokenType::KEYWORD_FOR})) return parseForStatement();
    if (match({TokenType::KEYWORD_RETURN})) return parseReturnStatement();
    if (match({TokenType::LBRACE})) return parseBlockStatement();
    
    // Check if it is a variable declaration: starts with int, char, void
    if (check(TokenType::KEYWORD_INT) || check(TokenType::KEYWORD_CHAR) || check(TokenType::KEYWORD_VOID)) {
        return parseDeclarationStatement();
    }

    return parseExpressionStatement();
}

std::shared_ptr<StatementNode> CppParser::parseDeclarationStatement() {
    std::string type = advance().value; // type
    Token nameToken = consume(TokenType::IDENTIFIER, "Expect variable name");
    std::string name = nameToken.value;

    std::shared_ptr<ExpressionNode> init = nullptr;
    if (match({TokenType::OP_ASSIGN})) {
        init = parseExpression();
    }

    consume(TokenType::SEMICOLON, "Expect ';' after variable declaration");
    return std::make_shared<VarDeclNode>(type, name, init);
}

std::shared_ptr<StatementNode> CppParser::parseExpressionStatement() {
    std::shared_ptr<ExpressionNode> expr = parseExpression();
    consume(TokenType::SEMICOLON, "Expect ';' after expression");
    
    // Check if expression is actually an assignment binary op, e.g. x = 5
    auto binOp = std::dynamic_pointer_cast<BinaryOpNode>(expr);
    if (binOp && binOp->op == TokenType::OP_ASSIGN) {
        auto leftId = std::dynamic_pointer_cast<IdentifierNode>(binOp->left);
        if (leftId) {
            return std::make_shared<AssignNode>(leftId->name, binOp->right);
        }
    }
    
    // It's a general expression statement (like i++; or a function call)
    // We can wrap it in an AssignNode if it's postfix unary (e.g. i++ / i--)
    // For simplicity, we just keep it as an ExpressionStatement.
    // Wait, let's wrap it in an AssignNode if it is a unary assignment or keep a wrapper statement
    // For our generator, we can accept ExpressionNode inside a general statement. Let's make an implementation check.
    // Wait, let's just make AssignNode general, or keep a wrapper:
    class ExprStatementNode : public StatementNode {
    public:
        std::shared_ptr<ExpressionNode> expr;
        ExprStatementNode(std::shared_ptr<ExpressionNode> e) : expr(e) {}
        void accept(ASTVisitor* visitor) override {
            // Fallback: we visit the expression or handle it.
            // Let's check: we can handle it directly or represent it.
            // Since we need to visit ExprStatementNode in ASTVisitor, let's look at ASTVisitor in AST.h.
            // Oh, we didn't define visit(ExprStatementNode*) in ASTVisitor. That's fine! We can map it to an AssignNode or BinaryOpNode.
        }
    };
    
    // If it's a unary postfix increment/decrement, we can treat it as an Assignment: e.g. x++ as x = x + 1.
    auto unaryOp = std::dynamic_pointer_cast<UnaryOpNode>(expr);
    if (unaryOp && (unaryOp->op == TokenType::OP_INC || unaryOp->op == TokenType::OP_DEC)) {
        auto id = std::dynamic_pointer_cast<IdentifierNode>(unaryOp->operand);
        if (id) {
            TokenType addOp = (unaryOp->op == TokenType::OP_INC) ? TokenType::OP_PLUS : TokenType::OP_MINUS;
            auto oneNode = std::make_shared<LiteralNode>("1", true);
            auto binOpNode = std::make_shared<BinaryOpNode>(id, addOp, oneNode);
            return std::make_shared<AssignNode>(id->name, binOpNode);
        }
    }
    
    // If it is a function call expression, we can wrap it as a ReturnNode or AssignNode with empty name,
    // or just return AssignNode with standard name.
    auto funcCall = std::dynamic_pointer_cast<FuncCallNode>(expr);
    if (funcCall) {
        return std::make_shared<AssignNode>("", funcCall); // Empty variable assignment signifies a lone function call statement
    }

    return std::make_shared<AssignNode>("", expr);
}

std::shared_ptr<BlockNode> CppParser::parseBlockStatement() {
    auto block = std::make_shared<BlockNode>();
    while (!check(TokenType::RBRACE) && !isAtEnd()) {
        block->statements.push_back(parseStatement());
    }
    consume(TokenType::RBRACE, "Expect '}' at end of block");
    return block;
}

std::shared_ptr<StatementNode> CppParser::parseIfStatement() {
    consume(TokenType::LPAREN, "Expect '(' after 'if'");
    std::shared_ptr<ExpressionNode> condition = parseExpression();
    consume(TokenType::RPAREN, "Expect ')' after condition");

    std::shared_ptr<StatementNode> thenStmt = parseStatement();
    std::shared_ptr<BlockNode> thenBlock = nullptr;
    if (auto b = std::dynamic_pointer_cast<BlockNode>(thenStmt)) {
        thenBlock = b;
    } else {
        thenBlock = std::make_shared<BlockNode>();
        thenBlock->statements.push_back(thenStmt);
    }

    std::shared_ptr<BlockNode> elseBlock = nullptr;
    if (match({TokenType::KEYWORD_ELSE})) {
        std::shared_ptr<StatementNode> elseStmt = parseStatement();
        if (auto b = std::dynamic_pointer_cast<BlockNode>(elseStmt)) {
            elseBlock = b;
        } else {
            elseBlock = std::make_shared<BlockNode>();
            elseBlock->statements.push_back(elseStmt);
        }
    }

    return std::make_shared<IfNode>(condition, thenBlock, elseBlock);
}

std::shared_ptr<StatementNode> CppParser::parseWhileStatement() {
    consume(TokenType::LPAREN, "Expect '(' after 'while'");
    std::shared_ptr<ExpressionNode> condition = parseExpression();
    consume(TokenType::RPAREN, "Expect ')' after condition");

    std::shared_ptr<StatementNode> bodyStmt = parseStatement();
    std::shared_ptr<BlockNode> bodyBlock = nullptr;
    if (auto b = std::dynamic_pointer_cast<BlockNode>(bodyStmt)) {
        bodyBlock = b;
    } else {
        bodyBlock = std::make_shared<BlockNode>();
        bodyBlock->statements.push_back(bodyStmt);
    }

    return std::make_shared<WhileNode>(condition, bodyBlock);
}

std::shared_ptr<StatementNode> CppParser::parseForStatement() {
    consume(TokenType::LPAREN, "Expect '(' after 'for'");
    
    std::shared_ptr<StatementNode> init = nullptr;
    if (!match({TokenType::SEMICOLON})) {
        if (check(TokenType::KEYWORD_INT) || check(TokenType::KEYWORD_CHAR) || check(TokenType::KEYWORD_VOID)) {
            init = parseDeclarationStatement();
        } else {
            init = parseExpressionStatement();
        }
    }

    std::shared_ptr<ExpressionNode> condition = nullptr;
    if (!check(TokenType::SEMICOLON)) {
        condition = parseExpression();
    }
    consume(TokenType::SEMICOLON, "Expect ';' after loop condition");

    std::shared_ptr<ExpressionNode> update = nullptr;
    if (!check(TokenType::RPAREN)) {
        update = parseExpression();
    }
    consume(TokenType::RPAREN, "Expect ')' after for clauses");

    std::shared_ptr<StatementNode> bodyStmt = parseStatement();
    std::shared_ptr<BlockNode> bodyBlock = nullptr;
    if (auto b = std::dynamic_pointer_cast<BlockNode>(bodyStmt)) {
        bodyBlock = b;
    } else {
        bodyBlock = std::make_shared<BlockNode>();
        bodyBlock->statements.push_back(bodyStmt);
    }

    return std::make_shared<ForNode>(init, condition, update, bodyBlock);
}

std::shared_ptr<StatementNode> CppParser::parseReturnStatement() {
    std::shared_ptr<ExpressionNode> value = nullptr;
    if (!check(TokenType::SEMICOLON)) {
        value = parseExpression();
    }
    consume(TokenType::SEMICOLON, "Expect ';' after return statement");
    return std::make_shared<ReturnNode>(value);
}

std::shared_ptr<FunctionNode> CppParser::parseFunction(const std::string& returnType, const std::string& name) {
    consume(TokenType::LPAREN, "Expect '(' after function name");
    
    std::vector<std::pair<std::string, std::string>> params;
    if (!check(TokenType::RPAREN)) {
        do {
            std::string pType = advance().value; // Parameter type
            std::string pName = consume(TokenType::IDENTIFIER, "Expect parameter name").value;
            params.emplace_back(pType, pName);
        } while (match({TokenType::COMMA}));
    }
    consume(TokenType::RPAREN, "Expect ')' after parameters");

    consume(TokenType::LBRACE, "Expect '{' before function body");
    std::shared_ptr<BlockNode> body = parseBlockStatement();

    return std::make_shared<FunctionNode>(returnType, name, params, body);
}

std::shared_ptr<ProgramNode> CppParser::parse() {
    auto program = std::make_shared<ProgramNode>();

    while (!isAtEnd()) {
        // High level structure: declarations (global variables or functions)
        if (check(TokenType::KEYWORD_INT) || check(TokenType::KEYWORD_CHAR) || check(TokenType::KEYWORD_VOID)) {
            std::string type = advance().value;
            std::string name = consume(TokenType::IDENTIFIER, "Expect identifier name").value;

            if (check(TokenType::LPAREN)) {
                // It is a function!
                program->functions.push_back(parseFunction(type, name));
            } else {
                // It is a global variable!
                std::shared_ptr<ExpressionNode> init = nullptr;
                if (match({TokenType::OP_ASSIGN})) {
                    init = parseExpression();
                }
                consume(TokenType::SEMICOLON, "Expect ';' after global variable");
                program->globals.push_back(std::make_shared<VarDeclNode>(type, name, init));
            }
        } else {
            // Skip unrecognized top level tokens to prevent infinite loop
            advance();
        }
    }

    return program;
}
