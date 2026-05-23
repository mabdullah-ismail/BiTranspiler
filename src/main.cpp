#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <chrono>
#include <thread>

#include "common/Token.h"
#include "lexer/CppLexer.h"
#include "lexer/AsmLexer.h"
#include "parser/CppParser.h"
#include "parser/AsmParser.h"
#include "codegen/AsmPrinter.h"
#include "codegen/CppPrinter.h"
#include "warning/WarningEngine.h"

// ANSI Color codes
#define RESET   "\033[0m"
#define BOLD    "\033[1m"
#define DIM     "\033[2m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define BLUE    "\033[34m"
#define MAGENTA "\033[35m"
#define CYAN    "\033[36m"
#define WHITE   "\033[37m"

// ─────────────────────────────────────────────
//  Utility functions
// ─────────────────────────────────────────────

std::string readFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open file: " + path);
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

void writeFile(const std::string& path, const std::string& content) {
    std::ofstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Could not write to file: " + path);
    }
    file << content;
}

// Read multi-line input from the user until they type END on its own line
std::string readCodeFromTerminal() {
    std::string code;
    std::string line;
    while (std::getline(std::cin, line)) {
        if (line == "END") break;
        code += line + "\n";
    }
    return code;
}

// ─────────────────────────────────────────────
//  Banner & Menu
// ─────────────────────────────────────────────

void printBanner() {
    std::cout << "\n";
    std::cout << BOLD << CYAN << "  BiTranspiler" << RESET << " - C++ and x86 MASM Assembly Transpiler\n";
    std::cout << DIM << "  COAL Lab Project\n" << RESET;
    std::cout << "\n";
}

void showMenu() {
    std::cout << "\n";
    std::cout << "  " << BOLD << "1" << RESET << "  C++ --> Assembly\n";
    std::cout << "  " << BOLD << "2" << RESET << "  Assembly --> C++\n";
    std::cout << "  " << BOLD << "3" << RESET << "  Exit\n";
    std::cout << "\n";
    std::cout << "  Choice: ";
}

// ─────────────────────────────────────────────
//  Core transpilation logic
// ─────────────────────────────────────────────

std::string transpileCppToAsm(const std::string& src, WarningEngine& warnings) {
    CppLexer lexer(src);
    std::vector<Token> tokens = lexer.tokenize();
    CppParser parser(tokens);
    auto programAST = parser.parse();
    AsmPrinter printer;
    programAST->accept(&printer);
    return printer.getAsmCode();
}

std::string transpileAsmToCpp(const std::string& src, WarningEngine& warnings) {
    AsmLexer lexer(src);
    std::vector<Token> tokens = lexer.tokenize();
    AsmParser parser(tokens);
    auto programAST = parser.parse();
    warnings.addWarnings(parser.getWarnings());
    CppPrinter printer;
    programAST->accept(&printer);
    return printer.getCppCode();
}

// ─────────────────────────────────────────────
//  Interactive handler
// ─────────────────────────────────────────────

void handlePasteAndTranspile(bool cppToAsm) {
    std::string language = cppToAsm ? "C++" : "MASM Assembly";

    std::cout << "\n";
    std::cout << "  Paste your " << language << " code below.\n";
    std::cout << "  Type " << BOLD << "END" << RESET << " on a new line when done.\n";
    std::cout << "\n";

    // Consume the leftover newline from menu input
    std::cin.ignore();

    std::string src = readCodeFromTerminal();

    if (src.empty() || src.find_first_not_of(" \t\n\r") == std::string::npos) {
        std::cout << "  " << RED << "Error: No code entered." << RESET << "\n";
        return;
    }

    try {
        auto start = std::chrono::high_resolution_clock::now();
        WarningEngine warnings;

        std::string result;
        if (cppToAsm) {
            result = transpileCppToAsm(src, warnings);
        } else {
            result = transpileAsmToCpp(src, warnings);
        }

        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> duration = end - start;

        // Print output
        std::cout << "\n";
        std::cout << "  " << GREEN << "Done" << RESET << " (" << duration.count() << " ms)\n";
        std::cout << "\n";
        std::cout << result;
        std::cout << "\n";

        // Print warnings if any
        if (warnings.hasWarnings()) {
            warnings.printSummary();
        }

        // Offer to save
        std::cout << "  Save to file? (filename or Enter to skip): ";
        std::string savePath;
        std::getline(std::cin, savePath);

        // Trim whitespace
        size_t startPos = savePath.find_first_not_of(" \t");
        if (startPos != std::string::npos) {
            savePath = savePath.substr(startPos);
            size_t endPos = savePath.find_last_not_of(" \t");
            if (endPos != std::string::npos) savePath = savePath.substr(0, endPos + 1);
        } else {
            savePath = "";
        }

        if (!savePath.empty()) {
            writeFile(savePath, result);
            std::cout << "  " << GREEN << "Saved to " << savePath << RESET << "\n";
        }

    } catch (const std::exception& e) {
        std::cout << "\n  " << RED << "Error: " << e.what() << RESET << "\n";
    }
}

// ─────────────────────────────────────────────
//  Main entry point
// ─────────────────────────────────────────────

int main(int argc, char* argv[]) {
    // Pipe mode — used by server.js and external tools
    // Usage: BiTranspiler.exe --cpp2asm < input.cpp
    //        BiTranspiler.exe --asm2cpp < input.asm
    if (argc > 1) {
        std::string flag = argv[1];
        if (flag == "--cpp2asm" || flag == "--asm2cpp") {
            std::stringstream buffer;
            buffer << std::cin.rdbuf();
            std::string src = buffer.str();
            try {
                if (flag == "--cpp2asm") {
                    CppLexer lexer(src);
                    std::vector<Token> tokens = lexer.tokenize();
                    CppParser parser(tokens);
                    auto programAST = parser.parse();
                    AsmPrinter printer;
                    programAST->accept(&printer);
                    std::cout << printer.getAsmCode();
                } else {
                    AsmLexer lexer(src);
                    std::vector<Token> tokens = lexer.tokenize();
                    AsmParser parser(tokens);
                    auto programAST = parser.parse();
                    CppPrinter printer;
                    programAST->accept(&printer);
                    for (const auto& w : parser.getWarnings()) {
                        std::cerr << "Warning: " << w << "\n";
                    }
                    std::cout << printer.getCppCode();
                }
                return 0;
            } catch (const std::exception& e) {
                std::cerr << "Error: " << e.what() << std::endl;
                return 1;
            }
        }
    }

    // Interactive mode
    printBanner();

    while (true) {
        showMenu();
        int choice;
        if (!(std::cin >> choice)) {
            std::cin.clear();
            std::string discard;
            std::cin >> discard;
            std::cout << "  " << RED << "Invalid input." << RESET << "\n";
            continue;
        }

        switch (choice) {
            case 1:
                handlePasteAndTranspile(true);  // C++ -> ASM
                break;
            case 2:
                handlePasteAndTranspile(false); // ASM -> C++
                break;
            case 3:
                std::cout << "\n  Goodbye.\n\n";
                return 0;
            default:
                std::cout << "  " << RED << "Choose 1, 2, or 3." << RESET << "\n";
                break;
        }
    }
}

