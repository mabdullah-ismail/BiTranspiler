# BiTranspiler — Bidirectional C++ ↔ x86 MASM Assembly Transpiler

A bidirectional transpiler that translates code between **C++17** and **32-bit x86 MASM Assembly** in both directions. Built as a COAL (Computer Organization and Assembly Language) lab project.

---

## Project Structure

```
Coal Lab Project/
├── src/                          # C++ source code for the transpiler engine
│   ├── main.cpp                  # CLI entry point (interactive menu)
│   ├── common/
│   │   ├── AST.h                 # Abstract Syntax Tree node definitions
│   │   └── Token.h               # Token types for both C++ and MASM
│   ├── lexer/
│   │   ├── CppLexer.cpp/h        # Tokenizer for C++ source code
│   │   └── AsmLexer.cpp/h        # Tokenizer for MASM assembly code
│   ├── parser/
│   │   ├── CppParser.cpp/h       # C++ → AST parser
│   │   └── AsmParser.cpp/h       # MASM → AST parser (with peephole optimizer)
│   └── codegen/
│       ├── AsmPrinter.cpp/h      # AST → MASM code generator
│       └── CppPrinter.cpp/h      # AST → C++ code generator
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

## How It Works

The transpiler uses a **shared Abstract Syntax Tree (AST)** as the intermediate representation:

```
C++ Source Code  →  [CppLexer → CppParser]  →  AST  →  [AsmPrinter]  →  MASM Assembly
MASM Assembly    →  [AsmLexer → AsmParser]  →  AST  →  [CppPrinter]  →  C++ Source Code
```

**Key techniques:**
- **Lexical analysis** tokenizes both C++ and MASM into a common token stream
- **Recursive descent parsing** builds the AST from tokens
- **Peephole optimization** in the ASM→C++ path eliminates redundant register variables
- **Condition simplification** recovers clean `if (a > b)` from `cmp/setg/movzx/je` sequences
- **Control flow recovery** maps flat JMP/label patterns back to structured `if`, `while`, `for` blocks

---

## Supported Language Features

| Feature | C++ → Assembly | Assembly → C++ |
|---------|:-:|:-:|
| Integer and char variables | ✓ | ✓ |
| Arithmetic (+, -, *, /) | ✓ | ✓ |
| Comparison operators (==, !=, <, >, <=, >=) | ✓ | ✓ |
| Bitwise operators (&, \|, ^, ~, <<, >>) | ✓ | ✓ |
| Logical operators (&&, \|\|, !) | ✓ | ✓ |
| if / if-else statements | ✓ | ✓ |
| while loops | ✓ | ✓ |
| for loops | ✓ | ✓ |
| Functions with parameters | ✓ | ✓ |
| Function calls (stdcall) | ✓ | ✓ |
| Global and local variables | ✓ | ✓ |
| Return values via EAX | ✓ | ✓ |

---

## Prerequisites

To **build from source**, you need one of:
- **Microsoft Visual Studio 2022** (with "Desktop Development with C++" workload)
- **MinGW / GCC** (g++ with C++17 support)

To **run the web GUI**, you need:
- **Node.js** (v18 or later)
- A C++ compiler accessible from PATH (for the "Run C++" feature)

> **Note:** A pre-built `BiTranspiler.exe` is included, so you can skip the build step if you just want to run it.

---



The CLI provides an interactive menu:
1. **C++ --> Assembly** — Paste your C++ code, type `END` on a new line, get Assembly output
2. **Assembly --> C++** — Paste your MASM Assembly, type `END` on a new line, get C++ output
3. **Exit**

The executable also supports **pipe mode** for automation:
```bash
BiTranspiler.exe --cpp2asm < input.cpp > output.asm
BiTranspiler.exe --asm2cpp < input.asm > output.cpp
```

### Building from Source

```bash
# Run the build script (auto-detects MSVC or GCC)
build.bat
```

---

## Test Cases

The `tests/` directory contains verified input/output pairs:

| Test | Input | Transpiled Output | Recovered C++ |
|------|-------|-------------------|---------------|
| Arithmetic | `test1_arithmetic.cpp` | `test1_arithmetic_out.asm` | `test1_arithmetic_recovered.cpp` |
| Factorial | `test2_factorial.cpp` | `test2_factorial_out.asm` | `test2_factorial_recovered.cpp` |
| Fibonacci | `test3_fibonacci.cpp` | `test3_fibonacci_out.asm` | `test3_fibonacci_recovered.cpp` |

---

## Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                        Web GUI (Browser)                        │
│   index.html + index.css + index.js                             │
│   Editors │ Templates │ Run Output │ Tutorial │ FAQs            │
└──────────────────────────┬──────────────────────────────────────┘
                           │ HTTP POST /api/transpile
                           │ HTTP POST /api/run
                           ▼
┌─────────────────────────────────────────────────────────────────┐
│                    Node.js Server (server.js)                   │
│   Static file serving │ Transpile API │ Compile & Run API       │
└──────────────────────────┬──────────────────────────────────────┘
                           │ stdin/stdout pipe
                           ▼
┌─────────────────────────────────────────────────────────────────┐
│                  BiTranspiler.exe (C++ Engine)                   │
│                                                                 │
│  ┌───────────┐    ┌───────────┐    ┌────────────────────────┐   │
│  │ CppLexer  │───▶│ CppParser │───▶│                        │   │
│  └───────────┘    └───────────┘    │    Shared AST           │   │
│                                    │    (Abstract Syntax     │   │
│  ┌───────────┐    ┌───────────┐    │     Tree)               │   │
│  │ AsmLexer  │───▶│ AsmParser │───▶│                        │   │
│  └───────────┘    └───────────┘    └──────────┬─────────────┘   │
│                                               │                 │
│                              ┌────────────────┴──────────┐      │
│                              ▼                           ▼      │
│                        ┌───────────┐              ┌───────────┐ │
│                        │AsmPrinter │              │CppPrinter │ │
│                        └───────────┘              └───────────┘ │
└─────────────────────────────────────────────────────────────────┘
```

---

## Author

Muhammad Abdullah Ismail

---


