#  BiTranspiler — Bidirectional C++ ↔ x86 MASM Assembly Transpiler

[![Language](https://img.shields.io/badge/Language-C++17-blue.svg)](https://en.cppreference.com/w/cpp/17)
[![Platform](https://img.shields.io/badge/Platform-Windows-lightgrey.svg)](https://microsoft.com)
[![Assembler](https://img.shields.io/badge/Assembler-32--bit%20MASM-red.svg)](https://learn.microsoft.com/en-us/cpp/assembler/masm/microsoft-macro-assembler-reference)
[![License](https://img.shields.io/badge/License-MIT-green.svg)](LICENSE)

A high-performance bidirectional transpiler that seamlessly translates code between **C++17** and **32-bit x86 MASM Assembly** in both directions. Built as a foundational Computer Organization and Assembly Language (COAL) lab project.

---

##  Project Structure

```text
Coal Lab Project/
├── src/                          # C++ source code for the transpiler engine
│   ├── main.cpp                  # CLI entry point (interactive menu)
│   ├── common/
│   │   ├── AST.h                 # Abstract Syntax Tree node definitions
│   │   └── Token.h               # Token types for both C++ and MASM
│   ├── lexer/
│   │   ├── CppLexer.cpp/.h       # Tokenizer for C++ source code
│   │   └── AsmLexer.cpp/.h       # Tokenizer for MASM assembly code
│   ├── parser/
│   │   ├── CppParser.cpp/.h      # C++ → AST parser
│   │   └── AsmParser.cpp/.h      # MASM → AST parser (with peephole optimizer)
│   └── codegen/
│       ├── AsmPrinter.cpp/.h     # AST → MASM code generator
│       └── CppPrinter.cpp/.h     # AST → C++ code generator
├── public/                       # Web GUI frontend
│   ├── index.html                # Main page
│   ├── index.css                 # Stylesheet
│   └── index.js                  # Frontend logic (editors, API calls, templates)
├── tests/                        # Test input/output files
│   ├── cpp_inputs/               # C++ test cases with expected outputs
│   └── asm_inputs/               # Assembly test cases
├── server.js                     # Node.js web server (serves GUI + transpile/run APIs)
├── build.bat                     # Windows build script (auto-detects MSVC or GCC)
├── BiTranspiler.exe              # Pre-built transpiler executable
└── README.md                     # This file
```

---

##  How It Works

The transpiler utilizes a **shared Abstract Syntax Tree (AST)** as its primary intermediate representation:

```text
C++ Source Code  →  [CppLexer → CppParser]  →  AST  →  [AsmPrinter]  →  MASM Assembly
MASM Assembly   →  [AsmLexer → AsmParser]  →  AST  →  [CppPrinter]  →  C++ Source Code
```

### Key Compilation Techniques:
* **Lexical Analysis:** Tokenizes both high-level C++ and low-level MASM constructs into a uniform token stream.
* **Recursive Descent Parsing:** Efficiently maps language tokens directly into structural AST nodes.
* **Peephole Optimization:** Cleans up the `ASM → C++` translation layer to eliminate redundant register variables.
* **Condition Simplification:** Reconstructs intuitive code patterns like `if (a > b)` out of raw `cmp/setg/movzx/je` structural logic.
* **Control Flow Recovery:** Formulates flat assembly jumps and labels back into standard structured `if`, `while`, and `for` control loops.

---

##  Features Matrix

| Feature | C++ → Assembly | Assembly → C++ |
| :--- | :---: | :---: |
| Integer and Char Variables | ✓ | ✓ |
| Arithmetic Operators (`+`, `-`, `*`, `/`) | ✓ | ✓ |
| Comparison Operators (`==`, `!=`, `<`, `>`, `<=`, `>=`) | ✓ | ✓ |
| Bitwise Operators (`&`, `\|`, `^`, `~`, `<<`, `>>`) | ✓ | ✓ |
| Logical Operators (`&&`, `\|\|`, `!`) | ✓ | ✓ |
| `if` / `if-else` Statements | ✓ | ✓ |
| `while` Loops | ✓ | ✓ |
| `for` Loops | ✓ | ✓ |
| Functions with Parameters | ✓ | ✓ |
| Function Calls (`stdcall`) | ✓ | ✓ |
| Global and Local Variables | ✓ | ✓ |
| Return Values via `EAX` Register | ✓ | ✓ |

---

##  Prerequisites

### To Build from Source:
* **Microsoft Visual Studio 2022** (with the *"Desktop Development with C++"* workload configured) **OR**
* **MinGW / GCC** (with explicit `g++` C++17 language support)

### To Run the Web GUI:
* **Node.js** (v18 or newer installed)
* A valid C++ compiler exposed to your system environment variables (`PATH`) to enable the "Run C++" interface execution feature.

>  **Note:** A pre-built `BiTranspiler.exe` binary is shipped out of the box. You can skip explicit compilation phases if you wish to run the app directly.

---

## 📖 Usage Guide

### Using the CLI Interface
Running the engine directly launches an interactive command-line interface terminal wizard:
1. **C++ --> Assembly** — Paste target C++ block logic, enter `END` on a completely new terminal line, and receive generated MASM output.
2. **Assembly --> C++** — Paste system MASM code blocks, enter `END` on a completely new terminal line, and receive structured C++ output code.
3. **Exit**

### Automation Pipe Mode
The backend executable fully supports standard input/output streaming redirection pipelines:
```bash
BiTranspiler.exe --cpp2asm < input.cpp > output.asm
BiTranspiler.exe --asm2cpp < input.asm > output.cpp
```

### Building From Source
To trigger the automated build ecosystem configuration pipeline run:
```bash
# Triggers internal automated architecture build discovery matching MSVC or GCC
build.bat
```

---

##  Test Case Suite

The local `tests/` path maps out verified diagnostic input-to-output coverage pairings:

| Test Module Target | Input Source File | Transpiled Output Generation | Recovered Level Output |
| :--- | :--- | :--- | :--- |
| **Arithmetic** | `test1_arithmetic.cpp` | `test1_arithmetic_out.asm` | `test1_arithmetic_recovered.cpp` |
| **Factorial** | `test2_factorial.cpp` | `test2_factorial_out.asm` | `test2_factorial_recovered.cpp` |
| **Fibonacci** | `test3_fibonacci.cpp` | `test3_fibonacci_out.asm` | `test3_fibonacci_recovered.cpp` |

---

##  Architecture Blueprint

```text
┌─────────────────────────────────────────────────────────────────┐
│                        Web GUI (Browser)                        │
│               index.html + index.css + index.js                 │
│   Editors │ Templates │ Run Output │ Tutorial │ FAQs            │
└──────────────────────────┬──────────────────────────────────────┘
                           │ HTTP POST /api/transpile
                           │ HTTP POST /api/run
                           ▼
┌─────────────────────────────────────────────────────────────────┐
│                 Node.js Server (server.js)                      │
│   Static file serving │ Transpile API │ Compile & Run API       │
└──────────────────────────┬──────────────────────────────────────┘
                           │ stdin/stdout pipe
                           ▼
┌─────────────────────────────────────────────────────────────────┐
│                 BiTranspiler.exe (C++ Engine)                   │
│                                                                 │
│  ┌───────────┐    ┌───────────┐    ┌────────────────────────┐   │
│  │ CppLexer  │───▶│ CppParser │───▶│                        │   │
│  └───────────┘    └───────────┘    │        Shared AST      │   │
│                                    │    (Abstract Syntax    │   │
│  ┌───────────┐    ┌───────────┐    │         Tree)          │   │
│  │ AsmLexer  │───▶│ AsmParser │───▶│                        │   │
│  └───────────┘    └───────────┘    └──────────┬─────────────┘   │
│                                               │                 │
│                              ┌────────────────┴──────────┐      │
│                              ▼                           ▼      │
│                        ┌───────────┐               ┌───────────┐│
│                        │AsmPrinter │               │CppPrinter ││
│                        └───────────┘               └───────────┘│
└─────────────────────────────────────────────────────────────────┘
```

---

##  Author

* **Muhammad Abdullah Ismail** — *Core Architecture & Design Implementation*
