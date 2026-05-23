#include "AsmPrinter.h"
#include <algorithm>

AsmPrinter::AsmPrinter() : labelCounter(0), currentFunction(""), inExpression(false) {}

std::string AsmPrinter::getAsmCode() const {
    return asmCode.str();
}

std::string AsmPrinter::generateLabel(const std::string& prefix) {
    return prefix + "_" + std::to_string(labelCounter++);
}

// Helper to recursively find local variable declarations
static void scanLocalVars(ASTNode* node, std::vector<std::pair<std::string, std::string>>& locals) {
    if (!node) return;

    if (auto varDecl = dynamic_cast<VarDeclNode*>(node)) {
        locals.emplace_back(varDecl->type, varDecl->name);
    } else if (auto block = dynamic_cast<BlockNode*>(node)) {
        for (const auto& stmt : block->statements) {
            scanLocalVars(stmt.get(), locals);
        }
    } else if (auto ifNode = dynamic_cast<IfNode*>(node)) {
        scanLocalVars(ifNode->thenBlock.get(), locals);
        if (ifNode->elseBlock) {
            scanLocalVars(ifNode->elseBlock.get(), locals);
        }
    } else if (auto whileNode = dynamic_cast<WhileNode*>(node)) {
        scanLocalVars(whileNode->body.get(), locals);
    } else if (auto forNode = dynamic_cast<ForNode*>(node)) {
        scanLocalVars(forNode->init.get(), locals);
        scanLocalVars(forNode->body.get(), locals);
    }
}

void AsmPrinter::visit(LiteralNode* node) {
    if (node->isNumeric) {
        asmCode << "  mov eax, " << node->value << "\n";
    } else {
        // Character literal
        if (node->value.length() == 1) {
            asmCode << "  mov eax, " << (int)node->value[0] << " ; '" << node->value << "'\n";
        } else {
            // String literal address (not fully supported, but we handle it as warning/comment)
            asmCode << "  mov eax, OFFSET " << node->value << "\n";
        }
    }
}

void AsmPrinter::visit(IdentifierNode* node) {
    asmCode << "  mov eax, " << node->name << "\n";
}

void AsmPrinter::visit(BinaryOpNode* node) {
    auto litRight = dynamic_cast<LiteralNode*>(node->right.get());
    auto idRight = dynamic_cast<IdentifierNode*>(node->right.get());
    
    bool isSimpleRight = false;
    std::string rhsStr = "";
    if (litRight && litRight->isNumeric) {
        isSimpleRight = true;
        rhsStr = litRight->value;
    } else if (idRight) {
        isSimpleRight = true;
        rhsStr = idRight->name;
    }

    if (node->op == TokenType::OP_LOG_AND || node->op == TokenType::OP_LOG_OR) {
        isSimpleRight = false;
    }

    if (isSimpleRight) {
        // 1. Visit left (result in EAX)
        node->left->accept(this);

        // 2. Perform operation directly with the simple RHS
        switch (node->op) {
            case TokenType::OP_PLUS:
                asmCode << "  add eax, " << rhsStr << "\n";
                break;
            case TokenType::OP_MINUS:
                asmCode << "  sub eax, " << rhsStr << "\n";
                break;
            case TokenType::OP_MUL:
                asmCode << "  imul eax, " << rhsStr << "\n";
                break;
            case TokenType::OP_DIV:
                asmCode << "  mov ebx, " << rhsStr << "\n";
                asmCode << "  cdq\n";
                asmCode << "  idiv ebx\n";
                break;
            case TokenType::OP_BIT_AND:
                asmCode << "  and eax, " << rhsStr << "\n";
                break;
            case TokenType::OP_BIT_OR:
                asmCode << "  or eax, " << rhsStr << "\n";
                break;
            case TokenType::OP_BIT_XOR:
                asmCode << "  xor eax, " << rhsStr << "\n";
                break;
            case TokenType::OP_LSHIFT:
                if (litRight) {
                    asmCode << "  shl eax, " << rhsStr << "\n";
                } else {
                    asmCode << "  mov ecx, " << rhsStr << "\n";
                    asmCode << "  shl eax, cl\n";
                }
                break;
            case TokenType::OP_RSHIFT:
                if (litRight) {
                    asmCode << "  sar eax, " << rhsStr << "\n";
                } else {
                    asmCode << "  mov ecx, " << rhsStr << "\n";
                    asmCode << "  sar eax, cl\n";
                }
                break;
            case TokenType::OP_EQ:
                asmCode << "  cmp eax, " << rhsStr << "\n";
                asmCode << "  sete al\n";
                asmCode << "  movzx eax, al\n";
                break;
            case TokenType::OP_NE:
                asmCode << "  cmp eax, " << rhsStr << "\n";
                asmCode << "  setne al\n";
                asmCode << "  movzx eax, al\n";
                break;
            case TokenType::OP_LT:
                asmCode << "  cmp eax, " << rhsStr << "\n";
                asmCode << "  setl al\n";
                asmCode << "  movzx eax, al\n";
                break;
            case TokenType::OP_GT:
                asmCode << "  cmp eax, " << rhsStr << "\n";
                asmCode << "  setg al\n";
                asmCode << "  movzx eax, al\n";
                break;
            case TokenType::OP_LE:
                asmCode << "  cmp eax, " << rhsStr << "\n";
                asmCode << "  setle al\n";
                asmCode << "  movzx eax, al\n";
                break;
            case TokenType::OP_GE:
                asmCode << "  cmp eax, " << rhsStr << "\n";
                asmCode << "  setge al\n";
                asmCode << "  movzx eax, al\n";
                break;
            default:
                break;
        }
        return;
    }

    // --- FALLBACK GENERAL CASE (Stack-based evaluation) ---
    // 1. Visit left (result in EAX)
    node->left->accept(this);
    // 2. Push EAX
    asmCode << "  push eax\n";
    // 3. Visit right (result in EAX)
    node->right->accept(this);
    // 4. Move right operand to EBX
    asmCode << "  mov ebx, eax\n";
    // 5. Pop left operand to EAX
    asmCode << "  pop eax\n";

    // 6. Perform operation
    switch (node->op) {
        case TokenType::OP_PLUS:
            asmCode << "  add eax, ebx\n";
            break;
        case TokenType::OP_MINUS:
            asmCode << "  sub eax, ebx\n";
            break;
        case TokenType::OP_MUL:
            asmCode << "  imul eax, ebx\n";
            break;
        case TokenType::OP_DIV:
            asmCode << "  cdq\n";
            asmCode << "  idiv ebx\n";
            break;
        case TokenType::OP_BIT_AND:
            asmCode << "  and eax, ebx\n";
            break;
        case TokenType::OP_BIT_OR:
            asmCode << "  or eax, ebx\n";
            break;
        case TokenType::OP_BIT_XOR:
            asmCode << "  xor eax, ebx\n";
            break;
        case TokenType::OP_LSHIFT:
            asmCode << "  mov ecx, ebx\n";
            asmCode << "  shl eax, cl\n";
            break;
        case TokenType::OP_RSHIFT:
            asmCode << "  mov ecx, ebx\n";
            asmCode << "  sar eax, cl\n";
            break;
        case TokenType::OP_EQ:
            asmCode << "  cmp eax, ebx\n";
            asmCode << "  sete al\n";
            asmCode << "  movzx eax, al\n";
            break;
        case TokenType::OP_NE:
            asmCode << "  cmp eax, ebx\n";
            asmCode << "  setne al\n";
            asmCode << "  movzx eax, al\n";
            break;
        case TokenType::OP_LT:
            asmCode << "  cmp eax, ebx\n";
            asmCode << "  setl al\n";
            asmCode << "  movzx eax, al\n";
            break;
        case TokenType::OP_GT:
            asmCode << "  cmp eax, ebx\n";
            asmCode << "  setg al\n";
            asmCode << "  movzx eax, al\n";
            break;
        case TokenType::OP_LE:
            asmCode << "  cmp eax, ebx\n";
            asmCode << "  setle al\n";
            asmCode << "  movzx eax, al\n";
            break;
        case TokenType::OP_GE:
            asmCode << "  cmp eax, ebx\n";
            asmCode << "  setge al\n";
            asmCode << "  movzx eax, al\n";
            break;
        case TokenType::OP_LOG_AND:
            asmCode << "  cmp eax, 0\n";
            asmCode << "  setne al\n";
            asmCode << "  movzx eax, al\n";
            asmCode << "  cmp ebx, 0\n";
            asmCode << "  setne bl\n";
            asmCode << "  movzx ebx, bl\n";
            asmCode << "  and eax, ebx\n";
            break;
        case TokenType::OP_LOG_OR:
            asmCode << "  cmp eax, 0\n";
            asmCode << "  setne al\n";
            asmCode << "  movzx eax, al\n";
            asmCode << "  cmp ebx, 0\n";
            asmCode << "  setne bl\n";
            asmCode << "  movzx ebx, bl\n";
            asmCode << "  or eax, ebx\n";
            break;
        case TokenType::OP_ASSIGN:
            break;
        default:
            asmCode << "  ; Unhandled binary operator\n";
            break;
    }
}

void AsmPrinter::visit(UnaryOpNode* node) {
    if (node->op == TokenType::OP_INC || node->op == TokenType::OP_DEC) {
        auto idNode = std::dynamic_pointer_cast<IdentifierNode>(node->operand);
        if (idNode) {
            std::string inst = (node->op == TokenType::OP_INC) ? "inc" : "dec";
            if (node->isPostfix) {
                // Postfix: Load old value, increment variable, return old value
                asmCode << "  mov eax, " << idNode->name << "\n";
                asmCode << "  " << inst << " DWORD PTR [" << idNode->name << "]\n";
            } else {
                // Prefix: Increment variable, load new value
                asmCode << "  " << inst << " DWORD PTR [" << idNode->name << "]\n";
                asmCode << "  mov eax, " << idNode->name << "\n";
            }
        }
        return;
    }

    node->operand->accept(this);
    switch (node->op) {
        case TokenType::OP_MINUS:
            asmCode << "  neg eax\n";
            break;
        case TokenType::OP_BIT_NOT:
            asmCode << "  not eax\n";
            break;
        case TokenType::OP_LOG_NOT:
            asmCode << "  cmp eax, 0\n";
            asmCode << "  sete al\n";
            asmCode << "  movzx eax, al\n";
            break;
        default:
            asmCode << "  ; Unhandled unary operator\n";
            break;
    }
}

void AsmPrinter::visit(FuncCallNode* node) {
    // stdcall passes parameters from right to left
    for (auto it = node->args.rbegin(); it != node->args.rend(); ++it) {
        (*it)->accept(this);
        asmCode << "  push eax\n";
    }
    asmCode << "  call " << node->funcName << "\n";
}

void AsmPrinter::visit(BlockNode* node) {
    for (const auto& stmt : node->statements) {
        stmt->accept(this);
    }
}

void AsmPrinter::visit(VarDeclNode* node) {
    // Local declarations are printed in FunctionNode.
    // If initialized, we emit the assignment.
    if (node->initVal) {
        node->initVal->accept(this);
        asmCode << "  mov " << node->name << ", eax\n";
    }
}

void AsmPrinter::visit(AssignNode* node) {
    if (!node->varName.empty()) {
        node->val->accept(this);
        asmCode << "  mov " << node->varName << ", eax\n";
    } else {
        // Lone expression statement
        node->val->accept(this);
    }
}

void AsmPrinter::visit(IfNode* node) {
    std::string elseLabel = generateLabel("L_else");
    std::string endLabel = generateLabel("L_end");

    node->condition->accept(this);
    asmCode << "  cmp eax, 0\n";
    asmCode << "  je " << elseLabel << "\n";

    node->thenBlock->accept(this);
    asmCode << "  jmp " << endLabel << "\n";

    asmCode << elseLabel << ":\n";
    if (node->elseBlock) {
        node->elseBlock->accept(this);
    }

    asmCode << endLabel << ":\n";
}

void AsmPrinter::visit(WhileNode* node) {
    std::string startLabel = generateLabel("L_while_start");
    std::string endLabel = generateLabel("L_while_end");

    asmCode << startLabel << ":\n";
    node->condition->accept(this);
    asmCode << "  cmp eax, 0\n";
    asmCode << "  je " << endLabel << "\n";

    node->body->accept(this);
    asmCode << "  jmp " << startLabel << "\n";
    asmCode << endLabel << ":\n";
}

void AsmPrinter::visit(ForNode* node) {
    std::string startLabel = generateLabel("L_for_start");
    std::string endLabel = generateLabel("L_for_end");

    if (node->init) {
        node->init->accept(this);
    }

    asmCode << startLabel << ":\n";
    if (node->condition) {
        node->condition->accept(this);
        asmCode << "  cmp eax, 0\n";
        asmCode << "  je " << endLabel << "\n";
    }

    node->body->accept(this);

    if (node->update) {
        node->update->accept(this);
    }

    asmCode << "  jmp " << startLabel << "\n";
    asmCode << endLabel << ":\n";
}

void AsmPrinter::visit(ReturnNode* node) {
    if (node->val) {
        node->val->accept(this);
    }
    if (currentFunction == "main") {
        asmCode << "  invoke ExitProcess, eax\n";
    } else {
        asmCode << "  ret\n";
    }
}

void AsmPrinter::visit(FunctionNode* node) {
    currentFunction = node->name;
    
    // Scan body to find all local variable declarations
    std::vector<std::pair<std::string, std::string>> locals;
    scanLocalVars(node->body.get(), locals);

    asmCode << node->name << " PROC";
    
    // Print params
    if (!node->params.empty()) {
        for (size_t i = 0; i < node->params.size(); ++i) {
            asmCode << (i == 0 ? " " : ", ") << node->params[i].second << ":DWORD";
        }
    }
    asmCode << "\n";

    // Print LOCAL declarations (must immediately follow PROC line in MASM)
    for (const auto& local : locals) {
        asmCode << "  LOCAL " << local.second << ":DWORD\n";
    }

    node->body->accept(this);

    // If main function doesn't end with return statement explicitly, we add default exit
    if (node->name == "main") {
        // Check if there was a return already
        std::string codeStr = asmCode.str();
        if (codeStr.find("ExitProcess") == std::string::npos) {
            asmCode << "  mov eax, 0\n";
            asmCode << "  invoke ExitProcess, eax\n";
        }
    } else {
        asmCode << "  ret\n";
    }

    asmCode << node->name << " ENDP\n\n";
    currentFunction = "";
}

void AsmPrinter::visit(ProgramNode* node) {
    asmCode << "; Generated by BiTranspiler\n";
    asmCode << ".386\n";
    asmCode << ".model flat, stdcall\n";
    asmCode << ".stack 4096\n";
    asmCode << "ExitProcess PROTO, dwExitCode:DWORD\n\n";

    // Data segment
    asmCode << ".data\n";
    for (const auto& global : node->globals) {
        std::string initValue = "0";
        if (global->initVal) {
            if (auto lit = std::dynamic_pointer_cast<LiteralNode>(global->initVal)) {
                initValue = lit->value;
            }
        }
        asmCode << "  " << global->name << " DD " << initValue << "\n";
    }
    asmCode << "\n";

    // Code segment
    asmCode << ".code\n";
    for (const auto& func : node->functions) {
        func->accept(this);
    }

    // End directive pointing to main entry point if present
    bool hasMain = std::any_of(node->functions.begin(), node->functions.end(), [](const auto& f) {
        return f->name == "main";
    });
    if (hasMain) {
        asmCode << "END main\n";
    } else {
        asmCode << "END\n";
    }
}
