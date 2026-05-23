#include "AsmParser.h"
#include <stdexcept>
#include <algorithm>
#include <iostream>
#include <map>

static bool isRegister(const std::string& arg) {
    static const std::vector<std::string> regs = {
        "eax", "ebx", "ecx", "edx", "esi", "edi", "ebp", "esp",
        "ax", "bx", "cx", "dx", "al", "bl", "cl", "dl", "ah", "bh", "ch", "dh"
    };
    return std::find(regs.begin(), regs.end(), arg) != regs.end();
}

static std::string cleanMemoryAccess(const std::string& arg) {
    size_t openBracket = arg.find('[');
    size_t closeBracket = arg.find(']');
    if (openBracket != std::string::npos && closeBracket != std::string::npos && closeBracket > openBracket) {
        std::string inner = arg.substr(openBracket + 1, closeBracket - openBracket - 1);
        // Trim leading and trailing spaces
        size_t first = inner.find_first_not_of(" \t");
        if (first == std::string::npos) return "";
        size_t last = inner.find_last_not_of(" \t");
        std::string trimmed = inner.substr(first, last - first + 1);
        
        // Remove spaces inside the trimmed string
        std::string cleanStr;
        for (char c : trimmed) {
            if (c != ' ' && c != '\t') {
                cleanStr += c;
            }
        }
        return cleanStr;
    }
    return arg;
}

static bool replaceIdentifier(std::shared_ptr<ExpressionNode>& node, const std::string& name, const std::shared_ptr<ExpressionNode>& newVal) {
    if (!node) return false;
    
    if (auto idNode = std::dynamic_pointer_cast<IdentifierNode>(node)) {
        if (idNode->name == name) {
            node = newVal;
            return true;
        }
    } else if (auto binOp = std::dynamic_pointer_cast<BinaryOpNode>(node)) {
        bool r1 = replaceIdentifier(binOp->left, name, newVal);
        bool r2 = replaceIdentifier(binOp->right, name, newVal);
        return r1 || r2;
    } else if (auto unaryOp = std::dynamic_pointer_cast<UnaryOpNode>(node)) {
        return replaceIdentifier(unaryOp->operand, name, newVal);
    } else if (auto call = std::dynamic_pointer_cast<FuncCallNode>(node)) {
        bool replaced = false;
        for (auto& arg : call->args) {
            if (replaceIdentifier(arg, name, newVal)) {
                replaced = true;
            }
        }
        return replaced;
    }
    return false;
}

static void simplifyCondition(std::shared_ptr<ExpressionNode>& node) {
    if (auto binOp = std::dynamic_pointer_cast<BinaryOpNode>(node)) {
        if (binOp->op == TokenType::OP_NE) {
            if (auto rightLit = std::dynamic_pointer_cast<LiteralNode>(binOp->right)) {
                if (rightLit->isNumeric && rightLit->value == "0") {
                    if (auto leftBin = std::dynamic_pointer_cast<BinaryOpNode>(binOp->left)) {
                        if (leftBin->op == TokenType::OP_EQ || leftBin->op == TokenType::OP_NE ||
                            leftBin->op == TokenType::OP_LT || leftBin->op == TokenType::OP_GT ||
                            leftBin->op == TokenType::OP_LE || leftBin->op == TokenType::OP_GE) {
                            node = leftBin;
                        }
                    }
                }
            }
        }
    }
}

static bool isIdentifierRead(const std::shared_ptr<ASTNode>& node, const std::string& name) {
    if (!node) return false;
    
    if (auto idNode = std::dynamic_pointer_cast<IdentifierNode>(node)) {
        return idNode->name == name;
    } else if (auto assignNode = std::dynamic_pointer_cast<AssignNode>(node)) {
        return isIdentifierRead(assignNode->val, name);
    } else if (auto declNode = std::dynamic_pointer_cast<VarDeclNode>(node)) {
        return isIdentifierRead(declNode->initVal, name);
    } else if (auto binOp = std::dynamic_pointer_cast<BinaryOpNode>(node)) {
        return isIdentifierRead(binOp->left, name) || isIdentifierRead(binOp->right, name);
    } else if (auto unaryOp = std::dynamic_pointer_cast<UnaryOpNode>(node)) {
        return isIdentifierRead(unaryOp->operand, name);
    } else if (auto callNode = std::dynamic_pointer_cast<FuncCallNode>(node)) {
        for (const auto& arg : callNode->args) {
            if (isIdentifierRead(arg, name)) return true;
        }
        return false;
    } else if (auto blockNode = std::dynamic_pointer_cast<BlockNode>(node)) {
        for (const auto& stmt : blockNode->statements) {
            if (isIdentifierRead(stmt, name)) return true;
        }
        return false;
    } else if (auto ifNode = std::dynamic_pointer_cast<IfNode>(node)) {
        return isIdentifierRead(ifNode->condition, name) ||
               isIdentifierRead(ifNode->thenBlock, name) ||
               isIdentifierRead(ifNode->elseBlock, name);
    } else if (auto whileNode = std::dynamic_pointer_cast<WhileNode>(node)) {
        return isIdentifierRead(whileNode->condition, name) ||
               isIdentifierRead(whileNode->body, name);
    } else if (auto forNode = std::dynamic_pointer_cast<ForNode>(node)) {
        return isIdentifierRead(forNode->init, name) ||
               isIdentifierRead(forNode->condition, name) ||
               isIdentifierRead(forNode->update, name) ||
               isIdentifierRead(forNode->body, name);
    } else if (auto retNode = std::dynamic_pointer_cast<ReturnNode>(node)) {
        return isIdentifierRead(retNode->val, name);
    }
    return false;
}

static void optimizeBlock(std::shared_ptr<BlockNode> block) {
    if (!block) return;
    
    // 1. Recursively optimize nested blocks
    for (auto& stmt : block->statements) {
        if (auto ifNode = std::dynamic_pointer_cast<IfNode>(stmt)) {
            optimizeBlock(ifNode->thenBlock);
            optimizeBlock(ifNode->elseBlock);
        } else if (auto whileNode = std::dynamic_pointer_cast<WhileNode>(stmt)) {
            optimizeBlock(whileNode->body);
        } else if (auto forNode = std::dynamic_pointer_cast<ForNode>(stmt)) {
            optimizeBlock(forNode->body);
        }
    }
    
    // 2. Perform copy propagation on registers in this block
    std::vector<std::shared_ptr<StatementNode>> newStmts;
    size_t i = 0;
    while (i < block->statements.size()) {
        auto stmt = block->statements[i];
        
        auto assignNode = std::dynamic_pointer_cast<AssignNode>(stmt);
        if (assignNode && isRegister(assignNode->varName) && i + 1 < block->statements.size()) {
            std::string regName = assignNode->varName;
            auto regVal = assignNode->val;
            auto nextStmt = block->statements[i + 1];
            
            // Case 1: Next is ReturnNode returning this register
            if (auto retNode = std::dynamic_pointer_cast<ReturnNode>(nextStmt)) {
                if (auto idNode = std::dynamic_pointer_cast<IdentifierNode>(retNode->val)) {
                    if (idNode->name == regName) {
                        retNode->val = regVal;
                        i++; // Skip register assignment
                        continue;
                    }
                }
            }
            
            // Case 2: Next is AssignNode using this register on RHS
            if (auto nextAssign = std::dynamic_pointer_cast<AssignNode>(nextStmt)) {
                // Next is var = reg
                if (auto idNode = std::dynamic_pointer_cast<IdentifierNode>(nextAssign->val)) {
                    if (idNode->name == regName) {
                        nextAssign->val = regVal;
                        i++; // Skip register assignment
                        continue;
                    }
                }
                // Next is var = func(..., reg, ...)
                if (auto callNode = std::dynamic_pointer_cast<FuncCallNode>(nextAssign->val)) {
                    bool replaced = false;
                    for (auto& arg : callNode->args) {
                        if (auto idNode = std::dynamic_pointer_cast<IdentifierNode>(arg)) {
                            if (idNode->name == regName) {
                                arg = regVal;
                                replaced = true;
                            }
                        }
                    }
                    if (replaced) {
                        i++; // Skip register assignment
                        continue;
                    }
                }
            }

            // Case 3: Next is IfNode whose condition references this register
            if (auto ifNode = std::dynamic_pointer_cast<IfNode>(nextStmt)) {
                if (replaceIdentifier(ifNode->condition, regName, regVal)) {
                    simplifyCondition(ifNode->condition);
                    i++; // Skip register assignment
                    continue;
                }
            }
            
            // Case 4: Next is WhileNode whose condition references this register
            if (auto whileNode = std::dynamic_pointer_cast<WhileNode>(nextStmt)) {
                if (replaceIdentifier(whileNode->condition, regName, regVal)) {
                    simplifyCondition(whileNode->condition);
                    i++; // Skip register assignment
                    continue;
                }
            }
        }
        
        newStmts.push_back(stmt);
        i++;
    }
    
    // 3. Dead register elimination pass (backward to handle dependency chains)
    std::vector<std::shared_ptr<StatementNode>> finalStmts = newStmts;
    for (int k = (int)finalStmts.size() - 1; k >= 0; --k) {
        auto stmt = finalStmts[k];
        if (!stmt) continue;
        
        auto assignNode = std::dynamic_pointer_cast<AssignNode>(stmt);
        if (assignNode && isRegister(assignNode->varName)) {
            std::string regName = assignNode->varName;
            bool isRead = false;
            for (size_t nextIdx = k + 1; nextIdx < finalStmts.size(); ++nextIdx) {
                if (finalStmts[nextIdx] && isIdentifierRead(finalStmts[nextIdx], regName)) {
                    isRead = true;
                    break;
                }
            }
            if (!isRead) {
                // Delete this statement by setting it to nullptr
                finalStmts[k] = nullptr;
            }
        }
    }
    
    // Filter out deleted null statements
    std::vector<std::shared_ptr<StatementNode>> nonNullStmts;
    for (const auto& stmt : finalStmts) {
        if (stmt) nonNullStmts.push_back(stmt);
    }
    block->statements = std::move(nonNullStmts);
}



static std::pair<std::string, std::string> resolveCmpArgs(const std::vector<AsmInstruction>& insts, int k) {
    std::map<std::string, std::string> regMap;
    std::vector<std::string> valStack;

    for (int i = 0; i < k; ++i) {
        const auto& inst = insts[i];
        if (inst.op == TokenType::INST_MOV && inst.args.size() >= 2) {
            std::string dest = inst.args[0];
            std::string src = inst.args[1];
            if (isRegister(dest)) {
                if (regMap.count(src)) {
                    regMap[dest] = regMap[src];
                } else {
                    regMap[dest] = src;
                }
            }
        } else if (inst.op == TokenType::INST_PUSH && !inst.args.empty()) {
            std::string val = inst.args[0];
            if (regMap.count(val)) {
                valStack.push_back(regMap[val]);
            } else {
                valStack.push_back(val);
            }
        } else if (inst.op == TokenType::INST_POP && !inst.args.empty()) {
            std::string reg = inst.args[0];
            if (isRegister(reg) && !valStack.empty()) {
                regMap[reg] = valStack.back();
                valStack.pop_back();
            }
        } else if ((inst.op == TokenType::INST_SETE || inst.op == TokenType::INST_SETNE ||
                    inst.op == TokenType::INST_SETG || inst.op == TokenType::INST_SETL ||
                    inst.op == TokenType::INST_SETGE || inst.op == TokenType::INST_SETLE) && !inst.args.empty()) {
            std::string dest = inst.args[0];
            if (isRegister(dest)) {
                std::string condStr = "";
                for (int h = i - 1; h >= 0; --h) {
                    if (insts[h].op == TokenType::INST_CMP && insts[h].args.size() >= 2) {
                        std::string opStr;
                        switch (inst.op) {
                            case TokenType::INST_SETE:  opStr = "=="; break;
                            case TokenType::INST_SETNE: opStr = "!="; break;
                            case TokenType::INST_SETG:  opStr = ">"; break;
                            case TokenType::INST_SETL:  opStr = "<"; break;
                            case TokenType::INST_SETGE: opStr = ">="; break;
                            case TokenType::INST_SETLE: opStr = "<="; break;
                            default:                    opStr = "=="; break;
                        }
                        std::string c1 = insts[h].args[0];
                        std::string c2 = insts[h].args[1];
                        if (regMap.count(c1)) c1 = regMap[c1];
                        if (regMap.count(c2)) c2 = regMap[c2];
                        condStr = "(" + c1 + " " + opStr + " " + c2 + ")";
                        break;
                    }
                }
                if (condStr.empty()) condStr = "1";
                regMap[dest] = condStr;
            }
        } else if (inst.op == TokenType::INST_MOVZX && inst.args.size() >= 2) {
            std::string dest = inst.args[0];
            std::string src = inst.args[1];
            if (isRegister(dest)) {
                if (regMap.count(src)) {
                    regMap[dest] = regMap[src];
                } else {
                    regMap[dest] = src;
                }
            }
        }
    }

    std::string arg1 = insts[k].args[0];
    std::string arg2 = insts[k].args.size() >= 2 ? insts[k].args[1] : "";
    if (regMap.count(arg1)) arg1 = regMap[arg1];
    if (regMap.count(arg2)) arg2 = regMap[arg2];

    return {arg1, arg2};
}

AsmParser::AsmParser(std::vector<Token> tkns) : tokens(std::move(tkns)), current(0) {}

Token AsmParser::peek() const {
    if (isAtEnd()) return tokens.back();
    return tokens[current];
}

Token AsmParser::peekNext() const {
    if (current + 1 >= tokens.size()) return tokens.back();
    return tokens[current + 1];
}

Token AsmParser::previous() const {
    if (current == 0) return tokens.front();
    return tokens[current - 1];
}

bool AsmParser::isAtEnd() const {
    return current >= tokens.size() || tokens[current].type == TokenType::EOF_TOKEN;
}

Token AsmParser::advance() {
    if (!isAtEnd()) current++;
    return previous();
}

bool AsmParser::check(TokenType type) const {
    if (isAtEnd()) return false;
    return peek().type == type;
}

bool AsmParser::match(const std::vector<TokenType>& types) {
    for (TokenType type : types) {
        if (check(type)) {
            advance();
            return true;
        }
    }
    return false;
}

Token AsmParser::consume(TokenType type, const std::string& message) {
    if (check(type)) return advance();
    throw std::runtime_error("AsmParser Error: " + message + " at line " + std::to_string(peek().line));
}

void AsmParser::skipNewlines() {
    // Newlines are skipped by the lexer in skipWhitespace()
}

std::vector<std::string> AsmParser::getWarnings() const {
    return warnings;
}

std::shared_ptr<ExpressionNode> AsmParser::makeExprFromArg(const std::string& arg) {
    // Helper to determine if an argument is a number or identifier/register
    if (arg.empty()) {
        return std::make_shared<LiteralNode>("0", true);
    }
    
    // Check if parenthesized expression (e.g. from resolved condition tracker)
    if (arg.front() == '(' && arg.back() == ')') {
        std::string inner = arg.substr(1, arg.length() - 2);
        static const std::vector<std::pair<std::string, TokenType>> ops = {
            {" == ", TokenType::OP_EQ},
            {" != ", TokenType::OP_NE},
            {" <= ", TokenType::OP_LE},
            {" >= ", TokenType::OP_GE},
            {" < ", TokenType::OP_LT},
            {" > ", TokenType::OP_GT}
        };
        for (const auto& opPair : ops) {
            size_t idx = inner.find(opPair.first);
            if (idx != std::string::npos) {
                std::string leftStr = inner.substr(0, idx);
                std::string rightStr = inner.substr(idx + opPair.first.length());
                return std::make_shared<BinaryOpNode>(makeExprFromArg(leftStr), opPair.second, makeExprFromArg(rightStr));
            }
        }
    }
    
    // Check if character literal e.g. 'A'
    if (arg.length() >= 2 && arg.front() == '\'' && arg.back() == '\'') {
        return std::make_shared<LiteralNode>(arg.substr(1, arg.length() - 2), false);
    }

    // Check if number (e.g. starts with digit or is hex with suffix h/H)
    bool isNum = true;
    // Check for hex suffix
    std::string cleanArg = arg;
    if ((arg.back() == 'h' || arg.back() == 'H') && arg.length() > 1) {
        // Parse hex
        cleanArg = arg.substr(0, arg.length() - 1);
        // check if rest are hex digits
        for (char c : cleanArg) {
            if (!std::isxdigit(c)) { isNum = false; break; }
        }
        if (isNum) {
            // Convert to decimal representation string
            try {
                unsigned int hexVal = std::stoul(cleanArg, nullptr, 16);
                return std::make_shared<LiteralNode>(std::to_string(hexVal), true);
            } catch (...) {
                isNum = false;
            }
        }
    } else {
        // check decimal
        for (size_t i = 0; i < arg.length(); ++i) {
            if (i == 0 && (arg[i] == '-' || arg[i] == '+')) continue;
            if (!std::isdigit(arg[i])) { isNum = false; break; }
        }
    }

    if (isNum) {
        return std::make_shared<LiteralNode>(cleanArg, true);
    }
    
    // Otherwise identifier or register
    return std::make_shared<IdentifierNode>(arg);
}

std::shared_ptr<ExpressionNode> AsmParser::recoverCondition(TokenType jumpOp, const std::string& arg1, const std::string& arg2) {
    auto left = makeExprFromArg(arg1);
    auto right = makeExprFromArg(arg2);
    
    // The condition is the inverse of the jump condition, because the jump targets
    // the label to skip the block if the condition is false.
    TokenType condOp;
    switch (jumpOp) {
        case TokenType::INST_JE:  condOp = TokenType::OP_NE; break;
        case TokenType::INST_JNE: condOp = TokenType::OP_EQ; break;
        case TokenType::INST_JG:  condOp = TokenType::OP_LE; break;
        case TokenType::INST_JL:  condOp = TokenType::OP_GE; break;
        case TokenType::INST_JGE: condOp = TokenType::OP_LT; break;
        case TokenType::INST_JLE: condOp = TokenType::OP_GT; break;
        default:                  condOp = TokenType::OP_NE; break;
    }
    return std::make_shared<BinaryOpNode>(left, condOp, right);
}

std::shared_ptr<BlockNode> AsmParser::recoverControlFlow(const std::vector<AsmInstruction>& insts, int start, int end) {
    auto block = std::make_shared<BlockNode>();
    
    int i = start;
    while (i < end) {
        const auto& inst = insts[i];
        
        // 1. Check for While Loop Pattern:
        // insts[i] has a label L_start
        // insts[j] is CMP
        // insts[j+1] is Jxx L_end
        // ...
        // insts[k] is JMP L_start
        // insts[k+1] has label L_end
        if (!inst.label.empty()) {
            std::string lStart = inst.label;
            int j = i;
            int cmpIdx = -1;
            int condJumpIdx = -1;
            int backJumpIdx = -1;
            
            // Search forward for conditional jump and backward jump
            for (int scan = i; scan < end; ++scan) {
                if (insts[scan].op == TokenType::INST_CMP) {
                    cmpIdx = scan;
                }
                if (scan > i && (insts[scan].op == TokenType::INST_JE || insts[scan].op == TokenType::INST_JNE ||
                                 insts[scan].op == TokenType::INST_JG || insts[scan].op == TokenType::INST_JL ||
                                 insts[scan].op == TokenType::INST_JGE || insts[scan].op == TokenType::INST_JLE)) {
                    condJumpIdx = scan;
                }
                if (insts[scan].op == TokenType::INST_JMP && !insts[scan].args.empty() && insts[scan].args[0] == lStart) {
                    backJumpIdx = scan;
                    break;
                }
            }
            
            if (condJumpIdx != -1 && backJumpIdx != -1 && condJumpIdx < backJumpIdx) {
                std::string lEnd = insts[condJumpIdx].args[0];
                // Check if target of back jump+1 matches lEnd
                bool foundEndLabel = false;
                int endLabelIdx = -1;
                for (int scan = backJumpIdx; scan < end; ++scan) {
                    if (insts[scan].label == lEnd || (scan + 1 < end && insts[scan + 1].label == lEnd)) {
                        foundEndLabel = true;
                        endLabelIdx = scan;
                        break;
                    }
                }
                
                if (foundEndLabel) {
                    // We found a while loop!
                    // Recover condition
                    std::shared_ptr<ExpressionNode> condition = nullptr;
                    int setIdx = -1;
                    for (int scan = i; scan < condJumpIdx; ++scan) {
                        TokenType op = insts[scan].op;
                        if (op == TokenType::INST_SETE || op == TokenType::INST_SETNE ||
                            op == TokenType::INST_SETG || op == TokenType::INST_SETL ||
                            op == TokenType::INST_SETGE || op == TokenType::INST_SETLE) {
                            setIdx = scan;
                            break;
                        }
                    }
                    if (setIdx != -1) {
                        int setCmpIdx = -1;
                        for (int scan = setIdx - 1; scan >= i; --scan) {
                            if (insts[scan].op == TokenType::INST_CMP) {
                                setCmpIdx = scan;
                                break;
                            }
                        }
                        if (setCmpIdx != -1 && insts[setCmpIdx].args.size() >= 2) {
                            TokenType condOp;
                            switch (insts[setIdx].op) {
                                case TokenType::INST_SETE:  condOp = TokenType::OP_EQ; break;
                                case TokenType::INST_SETNE: condOp = TokenType::OP_NE; break;
                                case TokenType::INST_SETG:  condOp = TokenType::OP_GT; break;
                                case TokenType::INST_SETL:  condOp = TokenType::OP_LT; break;
                                case TokenType::INST_SETGE: condOp = TokenType::OP_GE; break;
                                case TokenType::INST_SETLE: condOp = TokenType::OP_LE; break;
                                default:                    condOp = TokenType::OP_EQ; break;
                            }
                            auto resolved = resolveCmpArgs(insts, setCmpIdx);
                            condition = std::make_shared<BinaryOpNode>(
                                makeExprFromArg(resolved.first),
                                condOp,
                                makeExprFromArg(resolved.second)
                            );
                        }
                    }
                    if (!condition && cmpIdx != -1 && cmpIdx < condJumpIdx) {
                        const auto& cmpInst = insts[cmpIdx];
                        if (cmpInst.args.size() >= 2) {
                            auto resolved = resolveCmpArgs(insts, cmpIdx);
                            condition = recoverCondition(insts[condJumpIdx].op, resolved.first, resolved.second);
                        }
                    }
                    if (!condition) {
                        condition = std::make_shared<LiteralNode>("1", true); // fallback true
                    }
                    
                    // Body is from condJumpIdx + 1 to backJumpIdx
                    auto body = recoverControlFlow(insts, condJumpIdx + 1, backJumpIdx);
                    simplifyCondition(condition);
                    block->statements.push_back(std::make_shared<WhileNode>(condition, body));
                    
                    // Advance index past loop
                    i = endLabelIdx;
                    continue;
                }
            }
        }
        
        // 2. Check for Loop Instruction:
        // insts[i] has a loop target. We find LOOP L_loop backward.
        // Or if we encounter LOOP label forward, we match it.
        if (inst.op == TokenType::INST_LOOP && !inst.args.empty()) {
            std::string lLoop = inst.args[0];
            int h = -1;
            for (int scan = start; scan < i; ++scan) {
                if (insts[scan].label == lLoop) {
                    h = scan;
                    break;
                }
            }
            if (h != -1) {
                // Recover count: check for MOV ecx, count before label h
                std::string count = "10"; // default
                int initIdx = -1;
                for (int scan = h - 1; scan >= start; --scan) {
                    if (insts[scan].op == TokenType::INST_MOV && !insts[scan].args.empty() && insts[scan].args[0] == "ecx") {
                        count = insts[scan].args[1];
                        initIdx = scan;
                        break;
                    }
                }
                
                auto initStmt = std::make_shared<VarDeclNode>("int", "ecx", makeExprFromArg(count));
                auto cond = std::make_shared<BinaryOpNode>(std::make_shared<IdentifierNode>("ecx"), TokenType::OP_GT, std::make_shared<LiteralNode>("0", true));
                auto update = std::make_shared<UnaryOpNode>(TokenType::OP_DEC, std::make_shared<IdentifierNode>("ecx"), true);
                
                auto body = recoverControlFlow(insts, h, i);
                block->statements.push_back(std::make_shared<ForNode>(initStmt, cond, update, body));
                
                i++;
                continue;
            }
        }
        
        // 3. Check for If-Else Statement Pattern:
        // insts[i] is CMP
        // insts[i+1] is Jxx L_else
        // ...
        // insts[j] is JMP L_end
        // insts[j+1] has label L_else
        // ...
        // insts[k] has label L_end
        if (inst.op == TokenType::INST_CMP && i + 1 < end) {
            TokenType jumpOp = insts[i+1].op;
            if (jumpOp == TokenType::INST_JE || jumpOp == TokenType::INST_JNE || jumpOp == TokenType::INST_JG ||
                jumpOp == TokenType::INST_JL || jumpOp == TokenType::INST_JGE || jumpOp == TokenType::INST_JLE) {
                
                std::string lElse = insts[i+1].args[0];
                int j = i + 2;
                int elseLabelIdx = -1;
                int jmpEndIdx = -1;
                
                for (int scan = i + 2; scan < end; ++scan) {
                    if (insts[scan].label == lElse) {
                        elseLabelIdx = scan;
                        // check if previous is JMP to end
                        if (scan - 1 >= start && insts[scan-1].op == TokenType::INST_JMP) {
                            jmpEndIdx = scan - 1;
                        }
                        break;
                    }
                }
                
                if (elseLabelIdx != -1) {
                    std::shared_ptr<ExpressionNode> condition = nullptr;
                    if (inst.args.size() >= 2) {
                        auto resolved = resolveCmpArgs(insts, i);
                        condition = recoverCondition(jumpOp, resolved.first, resolved.second);
                    } else {
                        condition = std::make_shared<LiteralNode>("1", true);
                    }
                    
                    std::shared_ptr<BlockNode> thenBlock = nullptr;
                    std::shared_ptr<BlockNode> elseBlock = nullptr;
                    int nextIdx = elseLabelIdx;
                    
                    if (jmpEndIdx != -1) {
                        // If-else
                        std::string lEnd = insts[jmpEndIdx].args[0];
                        int endLabelIdx = -1;
                        for (int scan = elseLabelIdx; scan < end; ++scan) {
                            if (insts[scan].label == lEnd) {
                                endLabelIdx = scan;
                                break;
                            }
                        }
                        
                        thenBlock = recoverControlFlow(insts, i + 2, jmpEndIdx);
                        elseBlock = recoverControlFlow(insts, elseLabelIdx, endLabelIdx != -1 ? endLabelIdx : end);
                        nextIdx = endLabelIdx != -1 ? endLabelIdx : end;
                    } else {
                        // If only
                        thenBlock = recoverControlFlow(insts, i + 2, elseLabelIdx);
                    }
                    
                    simplifyCondition(condition);
                    block->statements.push_back(std::make_shared<IfNode>(condition, thenBlock, elseBlock));
                    i = nextIdx;
                    continue;
                }
            }
        }
        
        // 4. Default instructions / assignments
        // Check for function call: push, push, call pattern
        if (inst.op == TokenType::INST_CALL && !inst.args.empty()) {
            std::string fName = inst.args[0];
            std::vector<std::shared_ptr<ExpressionNode>> args;
            int scan = i - 1;
            while (scan >= start && insts[scan].op == TokenType::INST_PUSH) {
                args.push_back(makeExprFromArg(insts[scan].args[0]));
                scan--;
            }
            // Reverse args because they were pushed right-to-left
            std::reverse(args.begin(), args.end());
            
            // Check if call result is stored in some variable next: e.g. MOV var, eax
            if (i + 1 < end && insts[i+1].op == TokenType::INST_MOV && !insts[i+1].args.empty() && insts[i+1].args[1] == "eax") {
                block->statements.push_back(std::make_shared<AssignNode>(insts[i+1].args[0], std::make_shared<FuncCallNode>(fName, args)));
                i += 2;
                continue;
            } else {
                block->statements.push_back(std::make_shared<AssignNode>("", std::make_shared<FuncCallNode>(fName, args)));
                i++;
                continue;
            }
        }
        
        // Skip pushes if they are followed by call (since we handled them)
        if (inst.op == TokenType::INST_PUSH) {
            bool followedByCall = false;
            for (int scan = i + 1; scan < end; ++scan) {
                if (insts[scan].op == TokenType::INST_CALL) { followedByCall = true; break; }
                if (insts[scan].op != TokenType::INST_PUSH) break;
            }
            if (followedByCall) {
                i++;
                continue;
            }
        }

        // Peephole Optimizer & Copy-Propagation for High-Level Expressions
        if (inst.op == TokenType::INST_MOV && inst.args.size() >= 2 && inst.args[0] == "eax") {
            // Pattern 1: Division (5 instructions: mov eax, x; cdq; mov reg, y; idiv reg; mov z, eax)
            if (i + 4 < end &&
                insts[i+1].op == TokenType::IDENTIFIER && (insts[i+1].comment == "cdq" || insts[i+1].comment == "CDQ") &&
                insts[i+2].op == TokenType::INST_MOV && insts[i+2].args.size() >= 2 && isRegister(insts[i+2].args[0]) &&
                insts[i+3].op == TokenType::INST_DIV && insts[i+3].args.size() >= 1 && insts[i+3].args[0] == insts[i+2].args[0] &&
                insts[i+4].op == TokenType::INST_MOV && insts[i+4].args.size() >= 2 && insts[i+4].args[1] == "eax") {
                
                auto lhs = makeExprFromArg(inst.args[1]);
                auto rhs = makeExprFromArg(insts[i+2].args[1]);
                auto binOp = std::make_shared<BinaryOpNode>(lhs, TokenType::OP_DIV, rhs);
                block->statements.push_back(std::make_shared<AssignNode>(insts[i+4].args[0], binOp));
                i += 5;
                continue;
            }
            
            // Pattern 1b: Division with setup reversed (5 instructions: mov eax, x; mov reg, y; cdq; idiv reg; mov z, eax)
            if (i + 4 < end &&
                insts[i+1].op == TokenType::INST_MOV && insts[i+1].args.size() >= 2 && isRegister(insts[i+1].args[0]) &&
                insts[i+2].op == TokenType::IDENTIFIER && (insts[i+2].comment == "cdq" || insts[i+2].comment == "CDQ") &&
                insts[i+3].op == TokenType::INST_DIV && insts[i+3].args.size() >= 1 && insts[i+3].args[0] == insts[i+1].args[0] &&
                insts[i+4].op == TokenType::INST_MOV && insts[i+4].args.size() >= 2 && insts[i+4].args[1] == "eax") {
                
                auto lhs = makeExprFromArg(inst.args[1]);
                auto rhs = makeExprFromArg(insts[i+1].args[1]);
                auto binOp = std::make_shared<BinaryOpNode>(lhs, TokenType::OP_DIV, rhs);
                block->statements.push_back(std::make_shared<AssignNode>(insts[i+4].args[0], binOp));
                i += 5;
                continue;
            }
            
            // Pattern 2: Division (4 instructions: mov eax, x; cdq; idiv y; mov z, eax)
            if (i + 3 < end &&
                insts[i+1].op == TokenType::IDENTIFIER && (insts[i+1].comment == "cdq" || insts[i+1].comment == "CDQ") &&
                insts[i+2].op == TokenType::INST_DIV && insts[i+2].args.size() >= 1 &&
                insts[i+3].op == TokenType::INST_MOV && insts[i+3].args.size() >= 2 && insts[i+3].args[1] == "eax") {
                
                auto lhs = makeExprFromArg(inst.args[1]);
                auto rhs = makeExprFromArg(insts[i+2].args[0]);
                auto binOp = std::make_shared<BinaryOpNode>(lhs, TokenType::OP_DIV, rhs);
                block->statements.push_back(std::make_shared<AssignNode>(insts[i+3].args[0], binOp));
                i += 4;
                continue;
            }

            // Pattern 3: Addition (3 instructions: mov eax, x; add eax, y; mov z, eax)
            if (i + 2 < end &&
                insts[i+1].op == TokenType::INST_ADD && insts[i+1].args.size() >= 2 && insts[i+1].args[0] == "eax" &&
                insts[i+2].op == TokenType::INST_MOV && insts[i+2].args.size() >= 2 && insts[i+2].args[1] == "eax") {
                
                auto lhs = makeExprFromArg(inst.args[1]);
                auto rhs = makeExprFromArg(insts[i+1].args[1]);
                auto binOp = std::make_shared<BinaryOpNode>(lhs, TokenType::OP_PLUS, rhs);
                block->statements.push_back(std::make_shared<AssignNode>(insts[i+2].args[0], binOp));
                i += 3;
                continue;
            }

            // Pattern 4: Subtraction (3 instructions: mov eax, x; sub eax, y; mov z, eax)
            if (i + 2 < end &&
                insts[i+1].op == TokenType::INST_SUB && insts[i+1].args.size() >= 2 && insts[i+1].args[0] == "eax" &&
                insts[i+2].op == TokenType::INST_MOV && insts[i+2].args.size() >= 2 && insts[i+2].args[1] == "eax") {
                
                auto lhs = makeExprFromArg(inst.args[1]);
                auto rhs = makeExprFromArg(insts[i+1].args[1]);
                auto binOp = std::make_shared<BinaryOpNode>(lhs, TokenType::OP_MINUS, rhs);
                block->statements.push_back(std::make_shared<AssignNode>(insts[i+2].args[0], binOp));
                i += 3;
                continue;
            }

            // Pattern 5: Multiplication (3 instructions: mov eax, x; imul eax, y; mov z, eax)
            if (i + 2 < end &&
                insts[i+1].op == TokenType::INST_MUL && insts[i+1].args.size() >= 2 && insts[i+1].args[0] == "eax" &&
                insts[i+2].op == TokenType::INST_MOV && insts[i+2].args.size() >= 2 && insts[i+2].args[1] == "eax") {
                
                auto lhs = makeExprFromArg(inst.args[1]);
                auto rhs = makeExprFromArg(insts[i+1].args[1]);
                auto binOp = std::make_shared<BinaryOpNode>(lhs, TokenType::OP_MUL, rhs);
                block->statements.push_back(std::make_shared<AssignNode>(insts[i+2].args[0], binOp));
                i += 3;
                continue;
            }

            // Pattern 6: Direct variable copy-propagation assignment (2 instructions: mov eax, x; mov y, eax)
            if (i + 1 < end && insts[i+1].op == TokenType::INST_MOV && insts[i+1].args.size() >= 2 && insts[i+1].args[1] == "eax") {
                block->statements.push_back(std::make_shared<AssignNode>(insts[i+1].args[0], makeExprFromArg(inst.args[1])));
                i += 2;
                continue;
            }
        }

        // Check for invoke directive: invoke FunctionName, arg1, arg2, ...
        bool isInvoke = false;
        std::string lowerOp = inst.comment;
        std::transform(lowerOp.begin(), lowerOp.end(), lowerOp.begin(), [](unsigned char c) { return std::tolower(c); });
        if (inst.op == TokenType::IDENTIFIER && lowerOp == "invoke" && !inst.args.empty()) {
            isInvoke = true;
        }

        if (isInvoke) {
            std::string fName = inst.args[0];
            std::vector<std::shared_ptr<ExpressionNode>> args;
            for (size_t k = 1; k < inst.args.size(); ++k) {
                args.push_back(makeExprFromArg(inst.args[k]));
            }

            if (fName == "ExitProcess") {
                // Recover to return statement
                std::shared_ptr<ExpressionNode> retVal = nullptr;
                if (!args.empty()) {
                    retVal = args[0];
                } else {
                    retVal = std::make_shared<LiteralNode>("0", true);
                }
                block->statements.push_back(std::make_shared<ReturnNode>(retVal));
                i++;
                continue;
            } else {
                // General function call
                // Check if call result is stored in some variable next: e.g. MOV var, eax
                if (i + 1 < end && insts[i+1].op == TokenType::INST_MOV && !insts[i+1].args.empty() && insts[i+1].args[1] == "eax") {
                    block->statements.push_back(std::make_shared<AssignNode>(insts[i+1].args[0], std::make_shared<FuncCallNode>(fName, args)));
                    i += 2;
                    continue;
                } else {
                    block->statements.push_back(std::make_shared<AssignNode>("", std::make_shared<FuncCallNode>(fName, args)));
                    i++;
                    continue;
                }
            }
        }

        // Single Instructions mappings
        if (inst.op == TokenType::INST_MOV && inst.args.size() >= 2) {
            block->statements.push_back(std::make_shared<AssignNode>(inst.args[0], makeExprFromArg(inst.args[1])));
        } else if (inst.op == TokenType::INST_ADD && inst.args.size() >= 2) {
            auto id = std::make_shared<IdentifierNode>(inst.args[0]);
            auto rhs = makeExprFromArg(inst.args[1]);
            auto addNode = std::make_shared<BinaryOpNode>(id, TokenType::OP_PLUS, rhs);
            block->statements.push_back(std::make_shared<AssignNode>(inst.args[0], addNode));
        } else if (inst.op == TokenType::INST_SUB && inst.args.size() >= 2) {
            auto id = std::make_shared<IdentifierNode>(inst.args[0]);
            auto rhs = makeExprFromArg(inst.args[1]);
            auto subNode = std::make_shared<BinaryOpNode>(id, TokenType::OP_MINUS, rhs);
            block->statements.push_back(std::make_shared<AssignNode>(inst.args[0], subNode));
        } else if (inst.op == TokenType::INST_INC && !inst.args.empty()) {
            auto id = std::make_shared<IdentifierNode>(inst.args[0]);
            auto one = std::make_shared<LiteralNode>("1", true);
            auto addNode = std::make_shared<BinaryOpNode>(id, TokenType::OP_PLUS, one);
            block->statements.push_back(std::make_shared<AssignNode>(inst.args[0], addNode));
        } else if (inst.op == TokenType::INST_DEC && !inst.args.empty()) {
            auto id = std::make_shared<IdentifierNode>(inst.args[0]);
            auto one = std::make_shared<LiteralNode>("1", true);
            auto subNode = std::make_shared<BinaryOpNode>(id, TokenType::OP_MINUS, one);
            block->statements.push_back(std::make_shared<AssignNode>(inst.args[0], subNode));
        } else if (inst.op == TokenType::INST_NEG && !inst.args.empty()) {
            auto id = std::make_shared<IdentifierNode>(inst.args[0]);
            auto negNode = std::make_shared<UnaryOpNode>(TokenType::OP_MINUS, id);
            block->statements.push_back(std::make_shared<AssignNode>(inst.args[0], negNode));
        } else if (inst.op == TokenType::INST_MUL && !inst.args.empty()) {
            if (inst.args.size() >= 2) {
                auto id = std::make_shared<IdentifierNode>(inst.args[0]);
                auto rhs = makeExprFromArg(inst.args[1]);
                auto mulNode = std::make_shared<BinaryOpNode>(id, TokenType::OP_MUL, rhs);
                block->statements.push_back(std::make_shared<AssignNode>(inst.args[0], mulNode));
            } else {
                auto id = std::make_shared<IdentifierNode>("eax");
                auto rhs = makeExprFromArg(inst.args[0]);
                auto mulNode = std::make_shared<BinaryOpNode>(id, TokenType::OP_MUL, rhs);
                block->statements.push_back(std::make_shared<AssignNode>("eax", mulNode));
            }
        } else if (inst.op == TokenType::INST_DIV && !inst.args.empty()) {
            if (inst.args.size() >= 2) {
                auto id = std::make_shared<IdentifierNode>(inst.args[0]);
                auto rhs = makeExprFromArg(inst.args[1]);
                auto divNode = std::make_shared<BinaryOpNode>(id, TokenType::OP_DIV, rhs);
                block->statements.push_back(std::make_shared<AssignNode>(inst.args[0], divNode));
            } else {
                auto id = std::make_shared<IdentifierNode>("eax");
                auto rhs = makeExprFromArg(inst.args[0]);
                auto divNode = std::make_shared<BinaryOpNode>(id, TokenType::OP_DIV, rhs);
                block->statements.push_back(std::make_shared<AssignNode>("eax", divNode));
            }
        } else if ((inst.op == TokenType::INST_SETE || inst.op == TokenType::INST_SETNE ||
                    inst.op == TokenType::INST_SETG || inst.op == TokenType::INST_SETL ||
                    inst.op == TokenType::INST_SETGE || inst.op == TokenType::INST_SETLE) && !inst.args.empty()) {
            std::shared_ptr<ExpressionNode> rhs = nullptr;
            for (int k = i - 1; k >= start; --k) {
                if (insts[k].op == TokenType::INST_CMP && insts[k].args.size() >= 2) {
                    TokenType op;
                    switch (inst.op) {
                        case TokenType::INST_SETE:  op = TokenType::OP_EQ; break;
                        case TokenType::INST_SETNE: op = TokenType::OP_NE; break;
                        case TokenType::INST_SETG:  op = TokenType::OP_GT; break;
                        case TokenType::INST_SETL:  op = TokenType::OP_LT; break;
                        case TokenType::INST_SETGE: op = TokenType::OP_GE; break;
                        case TokenType::INST_SETLE: op = TokenType::OP_LE; break;
                        default:                    op = TokenType::OP_EQ; break;
                    }
                    auto left = makeExprFromArg(insts[k].args[0]);
                    auto right = makeExprFromArg(insts[k].args[1]);
                    rhs = std::make_shared<BinaryOpNode>(left, op, right);
                    break;
                }
            }
            if (!rhs) rhs = std::make_shared<LiteralNode>("1", true);
            block->statements.push_back(std::make_shared<AssignNode>(inst.args[0], rhs));
        } else if (inst.op == TokenType::INST_MOVZX && inst.args.size() >= 2) {
            block->statements.push_back(std::make_shared<AssignNode>(inst.args[0], makeExprFromArg(inst.args[1])));
        } else if (inst.op == TokenType::INST_RET) {
            block->statements.push_back(std::make_shared<ReturnNode>(makeExprFromArg("eax")));
        } else if (inst.op == TokenType::UNKNOWN) {
            // Unrecognized statement, record warning if it's not a pure label line
            if (inst.label.empty() && !inst.comment.empty()) {
                warnings.push_back("Line " + std::to_string(inst.line) + ": Unhandled instruction opcode: " + inst.comment);
            }
        }
        
        i++;
    }
    
    return block;
}

std::shared_ptr<ProgramNode> AsmParser::parse() {
    auto program = std::make_shared<ProgramNode>();
    
    while (!isAtEnd()) {
        skipNewlines();
        if (isAtEnd()) break;
        
        Token tok = peek();
        
        // Skip standard headers/directives
        if (tok.type == TokenType::DIR_MODEL || tok.type == TokenType::DIR_386 || tok.type == TokenType::DIR_STACK ||
            (tok.type == TokenType::IDENTIFIER && tok.value == "ExitProcess")) {
            // consume whole line
            while (!isAtEnd() && peek().line == tok.line) advance();
            continue;
        }
        
        // Parse .data segment
        if (tok.type == TokenType::DIR_DATA) {
            advance(); // consume .data
            while (!isAtEnd() && peek().type != TokenType::DIR_CODE && peek().type != TokenType::EOF_TOKEN) {
                if (check(TokenType::IDENTIFIER)) {
                    std::string varName = advance().value;
                    if (match({TokenType::DIR_DB, TokenType::DIR_DW, TokenType::DIR_DD})) {
                        std::string varType = "int"; // map DB/DW/DD to int/char
                        if (previous().type == TokenType::DIR_DB) varType = "char";
                        
                        std::string initVal = "0";
                        if (match({TokenType::NUMBER, TokenType::STRING})) {
                            initVal = previous().value;
                        }
                        
                        auto decl = std::make_shared<VarDeclNode>(varType, varName, makeExprFromArg(initVal));
                        program->globals.push_back(decl);
                    }
                } else {
                    advance(); // skip unrecognized token in .data
                }
            }
            continue;
        }
        
        // Parse .code segment
        if (tok.type == TokenType::DIR_CODE) {
            advance(); // consume .code
            continue;
        }
        
        // Parse procedures
        if (tok.type == TokenType::IDENTIFIER) {
            std::string procName = tok.value;
            int procLine = tok.line;
            advance(); // consume name
            
            if (match({TokenType::DIR_PROC})) {
                // Read PROC signature (e.g. arguments)
                std::vector<std::pair<std::string, std::string>> params;
                while (!isAtEnd() && peek().line == procLine) {
                    if (match({TokenType::COMMA})) continue;
                    if (check(TokenType::IDENTIFIER)) {
                        std::string pName = advance().value;
                        if (match({TokenType::COLON}) && match({TokenType::IDENTIFIER})) {
                            // paramName:DWORD
                            params.emplace_back("int", pName);
                        }
                    } else {
                        advance();
                    }
                }
                
                // Read all instructions inside the PROC until ENDP
                std::vector<AsmInstruction> procInsts;
                std::vector<std::pair<std::string, std::string>> locals;
                
                while (!isAtEnd()) {
                    skipNewlines();
                    if (isAtEnd()) break;
                    
                    // Check for ENDP
                    if (peek().type == TokenType::IDENTIFIER && peek().value == procName) {
                        size_t savePos = current;
                        advance(); // consume name
                        if (peek().type == TokenType::DIR_ENDP) {
                            advance(); // consume ENDP
                            break;
                        }
                        current = savePos; // backtrack
                    }
                    
                    AsmInstruction inst;
                    inst.line = peek().line;
                    
                    // Check for Label
                    if (peek().type == TokenType::IDENTIFIER && peekNext().type == TokenType::COLON) {
                        inst.label = advance().value;
                        advance(); // consume COLON
                    }
                    
                    // Check for Instruction or LOCAL directive
                    if (peek().type == TokenType::IDENTIFIER && peek().value == "LOCAL") {
                        advance(); // consume LOCAL
                        if (check(TokenType::IDENTIFIER)) {
                            std::string locName = advance().value;
                            if (match({TokenType::COLON}) && check(TokenType::IDENTIFIER)) {
                                std::string locType = advance().value;
                                locals.emplace_back(locType == "BYTE" ? "char" : "int", locName);
                            }
                        }
                        // consume rest of the line
                        while (!isAtEnd() && peek().line == inst.line) advance();
                        continue;
                    }
                    
                    // Read opcode
                    if (peek().type != TokenType::EOF_TOKEN && peek().line == inst.line) {
                        inst.op = peek().type;
                        inst.comment = peek().value; // store content for comment fallback
                        advance();
                    }
                    
                    // Read operands
                    while (!isAtEnd() && peek().line == inst.line) {
                        if (match({TokenType::COMMA})) continue;
                        
                        // Parse operand argument (can be multiple tokens, e.g. DWORD PTR [ebp-4])
                        std::string argStr;
                        int argLine = peek().line;
                        while (!isAtEnd() && peek().line == argLine && peek().type != TokenType::COMMA) {
                            argStr += (argStr.empty() ? "" : " ") + peek().value;
                            advance();
                        }
                        inst.args.push_back(cleanMemoryAccess(argStr));
                    }
                    
                    procInsts.push_back(inst);
                }
                
                // Recover control flow block
                auto body = recoverControlFlow(procInsts, 0, (int)procInsts.size());
                
                // Optimize registers out of recovered control flow block
                optimizeBlock(body);
                
                // Add local variables to the body block
                std::vector<std::shared_ptr<StatementNode>> withLocals;
                for (const auto& local : locals) {
                    withLocals.push_back(std::make_shared<VarDeclNode>(local.first, local.second, nullptr));
                }
                // Append original recovered statements
                withLocals.insert(withLocals.end(), body->statements.begin(), body->statements.end());
                body->statements = std::move(withLocals);
                
                auto function = std::make_shared<FunctionNode>("int", procName, params, body);
                program->functions.push_back(function);
                continue;
            }
        }
        
        advance(); // fallthrough safety
    }
    
    return program;
}
