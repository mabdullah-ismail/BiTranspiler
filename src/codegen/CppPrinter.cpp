#include "CppPrinter.h"
#include <unordered_set>

static bool isRegisterName(const std::string& name) {
    static const std::unordered_set<std::string> regs = {
        "eax", "ebx", "ecx", "edx", "esi", "edi", "esp", "ebp",
        "ax", "bx", "cx", "dx", "al", "bl", "cl", "dl", "ah", "bh", "ch", "dh"
    };
    return regs.count(name) > 0;
}

static bool usesRegisters(const std::shared_ptr<ASTNode>& node) {
    if (!node) return false;
    
    if (auto block = std::dynamic_pointer_cast<BlockNode>(node)) {
        for (const auto& stmt : block->statements) {
            if (usesRegisters(stmt)) return true;
        }
    } else if (auto decl = std::dynamic_pointer_cast<VarDeclNode>(node)) {
        if (usesRegisters(decl->initVal)) return true;
    } else if (auto assign = std::dynamic_pointer_cast<AssignNode>(node)) {
        if (isRegisterName(assign->varName)) return true;
        if (usesRegisters(assign->val)) return true;
    } else if (auto ifNode = std::dynamic_pointer_cast<IfNode>(node)) {
        if (usesRegisters(ifNode->condition)) return true;
        if (usesRegisters(ifNode->thenBlock)) return true;
        if (usesRegisters(ifNode->elseBlock)) return true;
    } else if (auto whileNode = std::dynamic_pointer_cast<WhileNode>(node)) {
        if (usesRegisters(whileNode->condition)) return true;
        if (usesRegisters(whileNode->body)) return true;
    } else if (auto forNode = std::dynamic_pointer_cast<ForNode>(node)) {
        if (usesRegisters(forNode->init)) return true;
        if (usesRegisters(forNode->condition)) return true;
        if (usesRegisters(forNode->update)) return true;
        if (usesRegisters(forNode->body)) return true;
    } else if (auto retNode = std::dynamic_pointer_cast<ReturnNode>(node)) {
        if (usesRegisters(retNode->val)) return true;
    } else if (auto idNode = std::dynamic_pointer_cast<IdentifierNode>(node)) {
        if (isRegisterName(idNode->name)) return true;
    } else if (auto binOp = std::dynamic_pointer_cast<BinaryOpNode>(node)) {
        if (usesRegisters(binOp->left)) return true;
        if (usesRegisters(binOp->right)) return true;
    } else if (auto unaryOp = std::dynamic_pointer_cast<UnaryOpNode>(node)) {
        if (usesRegisters(unaryOp->operand)) return true;
    } else if (auto call = std::dynamic_pointer_cast<FuncCallNode>(node)) {
        for (const auto& arg : call->args) {
            if (usesRegisters(arg)) return true;
        }
    }
    
    return false;
}

CppPrinter::CppPrinter() : indentLevel(0) {}

std::string CppPrinter::getCppCode() const {
    return cppCode.str();
}

void CppPrinter::printIndent() {
    for (int i = 0; i < indentLevel; ++i) {
        cppCode << "    ";
    }
}

std::string CppPrinter::getOpString(TokenType op) {
    switch (op) {
        case TokenType::OP_ASSIGN: return " = ";
        case TokenType::OP_PLUS: return " + ";
        case TokenType::OP_MINUS: return " - ";
        case TokenType::OP_MUL: return " * ";
        case TokenType::OP_DIV: return " / ";
        case TokenType::OP_EQ: return " == ";
        case TokenType::OP_NE: return " != ";
        case TokenType::OP_LT: return " < ";
        case TokenType::OP_GT: return " > ";
        case TokenType::OP_LE: return " <= ";
        case TokenType::OP_GE: return " >= ";
        case TokenType::OP_BIT_AND: return " & ";
        case TokenType::OP_BIT_OR: return " | ";
        case TokenType::OP_BIT_XOR: return " ^ ";
        case TokenType::OP_LSHIFT: return " << ";
        case TokenType::OP_RSHIFT: return " >> ";
        case TokenType::OP_LOG_AND: return " && ";
        case TokenType::OP_LOG_OR: return " || ";
        default: return " ? ";
    }
}

void CppPrinter::visit(LiteralNode* node) {
    if (!node->isNumeric) {
        // String or char literal
        if (node->value.length() == 1) {
            cppCode << "'" << node->value << "'";
        } else {
            cppCode << "\"" << node->value << "\"";
        }
    } else {
        cppCode << node->value;
    }
}

void CppPrinter::visit(IdentifierNode* node) {
    cppCode << node->name;
}

void CppPrinter::visit(BinaryOpNode* node) {
    cppCode << "(";
    node->left->accept(this);
    cppCode << getOpString(node->op);
    node->right->accept(this);
    cppCode << ")";
}

void CppPrinter::visit(UnaryOpNode* node) {
    std::string opStr;
    switch (node->op) {
        case TokenType::OP_MINUS: opStr = "-"; break;
        case TokenType::OP_BIT_NOT: opStr = "~"; break;
        case TokenType::OP_LOG_NOT: opStr = "!"; break;
        case TokenType::OP_INC: opStr = "++"; break;
        case TokenType::OP_DEC: opStr = "--"; break;
        default: opStr = "?"; break;
    }

    if (node->isPostfix) {
        node->operand->accept(this);
        cppCode << opStr;
    } else {
        cppCode << opStr;
        node->operand->accept(this);
    }
}

void CppPrinter::visit(FuncCallNode* node) {
    cppCode << node->funcName << "(";
    for (size_t i = 0; i < node->args.size(); ++i) {
        if (i > 0) cppCode << ", ";
        node->args[i]->accept(this);
    }
    cppCode << ")";
}

void CppPrinter::visit(BlockNode* node) {
    for (const auto& stmt : node->statements) {
        // Don't print empty statement nodes
        auto assignNode = std::dynamic_pointer_cast<AssignNode>(stmt);
        if (assignNode && assignNode->varName.empty() && !dynamic_cast<FuncCallNode*>(assignNode->val.get())) {
            // It's a register load or unhandled statement that doesn't need to print in C++
            continue;
        }
        
        printIndent();
        stmt->accept(this);
        cppCode << "\n";
    }
}

void CppPrinter::visit(VarDeclNode* node) {
    cppCode << node->type << " " << node->name;
    if (node->initVal) {
        cppCode << " = ";
        node->initVal->accept(this);
    }
    cppCode << ";";
}

void CppPrinter::visit(AssignNode* node) {
    if (!node->varName.empty()) {
        cppCode << node->varName << " = ";
        node->val->accept(this);
    } else {
        // Lone function call or expression
        node->val->accept(this);
    }
    cppCode << ";";
}

void CppPrinter::visit(IfNode* node) {
    cppCode << "if ";
    node->condition->accept(this);
    cppCode << " {\n";
    
    indentLevel++;
    node->thenBlock->accept(this);
    indentLevel--;
    
    if (node->elseBlock && !node->elseBlock->statements.empty()) {
        printIndent();
        cppCode << "} else {\n";
        indentLevel++;
        node->elseBlock->accept(this);
        indentLevel--;
    }
    
    printIndent();
    cppCode << "}";
}

void CppPrinter::visit(WhileNode* node) {
    cppCode << "while ";
    node->condition->accept(this);
    cppCode << " {\n";
    
    indentLevel++;
    node->body->accept(this);
    indentLevel--;
    
    printIndent();
    cppCode << "}";
}

void CppPrinter::visit(ForNode* node) {
    cppCode << "for (";
    if (node->init) {
        // Visit standard declaration or assignment, strip ending semicolon
        std::stringstream temp;
        std::streambuf* oldBuf = cppCode.std::ios::rdbuf(temp.rdbuf());
        node->init->accept(this);
        cppCode.std::ios::rdbuf(oldBuf);
        std::string initStr = temp.str();
        if (!initStr.empty() && initStr.back() == ';') initStr.pop_back();
        cppCode << initStr;
    }
    cppCode << "; ";
    if (node->condition) {
        node->condition->accept(this);
    }
    cppCode << "; ";
    if (node->update) {
        node->update->accept(this);
    }
    cppCode << ") {\n";
    
    indentLevel++;
    node->body->accept(this);
    indentLevel--;
    
    printIndent();
    cppCode << "}";
}

void CppPrinter::visit(ReturnNode* node) {
    cppCode << "return";
    if (node->val) {
        cppCode << " ";
        node->val->accept(this);
    }
    cppCode << ";";
}

void CppPrinter::visit(FunctionNode* node) {
    cppCode << node->returnType << " " << node->name << "(";
    for (size_t i = 0; i < node->params.size(); ++i) {
        if (i > 0) cppCode << ", ";
        cppCode << node->params[i].first << " " << node->params[i].second;
    }
    cppCode << ") {\n";
    
    indentLevel++;
    if (usesRegisters(node->body)) {
        printIndent();
        cppCode << "int eax = 0, ebx = 0, ecx = 0, edx = 0, ax = 0, bx = 0, cx = 0, dx = 0, al = 0, bl = 0, cl = 0, dl = 0; // CPU registers as variables\n";
    }
    node->body->accept(this);
    indentLevel--;
    
    cppCode << "}\n\n";
}

void CppPrinter::visit(ProgramNode* node) {
    cppCode << "// Generated by BiTranspiler from MASM source\n";
    cppCode << "#include <iostream>\n";
    cppCode << "using namespace std;\n\n";

    // Global variables
    if (!node->globals.empty()) {
        for (const auto& global : node->globals) {
            global->accept(this);
            cppCode << "\n";
        }
        cppCode << "\n";
    }

    // Functions
    for (const auto& func : node->functions) {
        func->accept(this);
    }
}
