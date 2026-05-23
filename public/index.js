// DOM Elements
const cppEditor = document.getElementById('cpp-editor');
const asmEditor = document.getElementById('asm-editor');
const btnCppToAsm = document.getElementById('btn-cpp-to-asm');
const btnAsmToCpp = document.getElementById('btn-asm-to-cpp');
const templateSelect = document.getElementById('template-select');
const consoleLogs = document.getElementById('console-logs');
const clearConsole = document.getElementById('clear-console');
const btnClearEditors = document.getElementById('clear-editors');
const btnRunCpp = document.getElementById('btn-run-cpp');
const runOutput = document.getElementById('run-output');
const clearOutput = document.getElementById('clear-output');

// Templates definitions
const templates = {
    arithmetic: {
        cpp: `int main() {\n    int a = 10;\n    int b = 5;\n    int sum = a + b;\n    int diff = a - b;\n    int prod = a * b;\n    int quot = a / b;\n    return sum;\n}`,
        asm: `; Basic Arithmetic Example\n.386\n.model flat, stdcall\n.stack 4096\nExitProcess PROTO, dwExitCode:DWORD\n\n.data\n  a DD 10\n  b DD 5\n  sum DD 0\n  diff DD 0\n  prod DD 0\n  quot DD 0\n\n.code\nmain PROC\n  mov eax, a\n  add eax, b\n  mov sum, eax\n\n  mov eax, a\n  sub eax, b\n  mov diff, eax\n\n  mov eax, a\n  imul eax, b\n  mov prod, eax\n\n  mov eax, a\n  cdq\n  mov ebx, b\n  idiv ebx\n  mov quot, eax\n\n  mov eax, sum\n  invoke ExitProcess, eax\nmain ENDP\nEND main`
    },
    factorial: {
        cpp: `int factorial(int n) {\n    int result = 1;\n    int i = 1;\n    while (i <= n) {\n        result = result * i;\n        i++;\n    }\n    return result;\n}\n\nint main() {\n    int num = 5;\n    int fact = factorial(num);\n    return fact;\n}`,
        asm: `; Factorial Function Example\n.386\n.model flat, stdcall\n.stack 4096\nExitProcess PROTO, dwExitCode:DWORD\n\n.code\nfactorial PROC n:DWORD\n  LOCAL result:DWORD\n  LOCAL i:DWORD\n  mov eax, 1\n  mov result, eax\n  mov eax, 1\n  mov i, eax\nL_while_start_0:\n  mov eax, i\n  push eax\n  mov eax, n\n  mov ebx, eax\n  pop eax\n  cmp eax, ebx\n  setle al\n  movzx eax, al\n  cmp eax, 0\n  je L_while_end_1\n  mov eax, result\n  push eax\n  mov eax, i\n  mov ebx, eax\n  pop eax\n  imul eax, ebx\n  mov result, eax\n  mov eax, i\n  push eax\n  mov eax, 1\n  mov ebx, eax\n  pop eax\n  add eax, ebx\n  mov i, eax\n  jmp L_while_start_0\nL_while_end_1:\n  mov eax, result\n  ret\nfactorial ENDP\n\nmain PROC\n  LOCAL num:DWORD\n  LOCAL fact:DWORD\n  mov eax, 5\n  mov num, eax\n  mov eax, num\n  push eax\n  call factorial\n  mov fact, eax\n  mov eax, fact\n  invoke ExitProcess, eax\nmain ENDP\nEND main`
    },
    fibonacci: {
        cpp: `int fibonacci(int n) {\n    if (n <= 1) {\n        return n;\n    }\n    int prev2 = 0;\n    int prev1 = 1;\n    int current = 0;\n    for (int i = 2; i <= n; i++) {\n        current = prev1 + prev2;\n        prev2 = prev1;\n        prev1 = current;\n    }\n    return current;\n}\n\nint main() {\n    int val = fibonacci(7);\n    return val;\n}`,
        asm: `; Fibonacci Loop Example\n.386\n.model flat, stdcall\n.stack 4096\nExitProcess PROTO, dwExitCode:DWORD\n\n.code\nfibonacci PROC n:DWORD\n  LOCAL prev2:DWORD\n  LOCAL prev1:DWORD\n  LOCAL current:DWORD\n  LOCAL i:DWORD\n  mov eax, n\n  push eax\n  mov eax, 1\n  mov ebx, eax\n  pop eax\n  cmp eax, ebx\n  setle al\n  movzx eax, al\n  cmp eax, 0\n  je L_else_0\n  mov eax, n\n  ret\n  jmp L_end_1\nL_else_0:\nL_end_1:\n  mov eax, 0\n  mov prev2, eax\n  mov eax, 1\n  mov prev1, eax\n  mov eax, 0\n  mov current, eax\n  mov eax, 2\n  mov i, eax\nL_for_start_2:\n  mov eax, i\n  push eax\n  mov eax, n\n  mov ebx, eax\n  pop eax\n  cmp eax, ebx\n  setle al\n  movzx eax, al\n  cmp eax, 0\n  je L_for_end_3\n  mov eax, prev1\n  push eax\n  mov eax, prev2\n  mov ebx, eax\n  pop eax\n  add eax, ebx\n  mov current, eax\n  mov eax, prev1\n  mov prev2, eax\n  mov eax, current\n  mov prev1, eax\n  mov eax, i\n  inc DWORD PTR [i]\n  jmp L_for_start_2\nL_for_end_3:\n  mov eax, current\n  ret\nfibonacci ENDP\n\nmain PROC\n  LOCAL val:DWORD\n  mov eax, 7\n  push eax\n  call fibonacci\n  mov val, eax\n  mov eax, val\n  invoke ExitProcess, eax\nmain ENDP\nEND main`
    }
};

// Logger Helper
function log(msg, type = 'system') {
    const timestamp = new Date().toLocaleTimeString();
    const entry = document.createElement('div');
    entry.className = `log-entry ${type}-msg`;
    entry.innerText = `[${timestamp}] ${msg}`;
    consoleLogs.appendChild(entry);
    consoleLogs.scrollTop = consoleLogs.scrollHeight;
}

function logRun(msg, type = 'system') {
    const timestamp = new Date().toLocaleTimeString();
    const entry = document.createElement('div');
    entry.className = `log-entry ${type}-msg`;
    entry.innerText = `[${timestamp}] ${msg}`;
    runOutput.appendChild(entry);
    runOutput.scrollTop = runOutput.scrollHeight;
}

// Clear Logs
clearConsole.addEventListener('click', () => {
    consoleLogs.innerHTML = '';
    log('Logs cleared.');
});

clearOutput.addEventListener('click', () => {
    runOutput.innerHTML = '';
    logRun('Output window cleared.');
});

// Load Templates
templateSelect.addEventListener('change', (e) => {
    const key = e.target.value;
    if (templates[key]) {
        cppEditor.value = templates[key].cpp;
        asmEditor.value = templates[key].asm;
        log(`Loaded template: "${key}" into C++ and Assembly editors.`, 'system');
    }
});

// API Transpile helper
async function performTranspilation(code, direction, button) {
    if (!code.trim()) {
        log('Error: Source editor is empty. Cannot transpile.', 'error');
        return;
    }

    button.classList.add('loading');
    log(`Starting ${direction === 'cpp2asm' ? 'C++ to MASM' : 'MASM to C++'} translation...`, 'system');

    try {
        const start = performance.now();
        const response = await fetch('/api/transpile', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ code, direction })
        });

        const data = await response.json();
        const end = performance.now();
        const elapsed = (end - start).toFixed(1);

        if (data.success) {
            if (direction === 'cpp2asm') {
                asmEditor.value = data.result;
            } else {
                cppEditor.value = data.result;
            }

            log(`Transpiled successfully in ${elapsed} ms.`, 'success');
            
            // Print warning logs if any
            if (data.warnings && data.warnings.trim()) {
                const lines = data.warnings.split('\n');
                lines.forEach(ln => {
                    if (ln.trim()) log(ln, 'warning');
                });
            }
        } else {
            log(`Compilation failed: ${data.error}`, 'error');
        }
    } catch (err) {
        log(`Network Error: ${err.message}`, 'error');
    } finally {
        button.classList.remove('loading');
    }
}

// Bind Buttons
btnCppToAsm.addEventListener('click', () => {
    performTranspilation(cppEditor.value, 'cpp2asm', btnCppToAsm);
});

btnAsmToCpp.addEventListener('click', () => {
    performTranspilation(asmEditor.value, 'asm2cpp', btnAsmToCpp);
});

btnClearEditors.addEventListener('click', () => {
    cppEditor.value = '';
    asmEditor.value = '';
    templateSelect.value = '';
    log('Cleared both C++ and x86 MASM Assembly editors.', 'system');
});

btnRunCpp.addEventListener('click', async () => {
    const code = cppEditor.value;
    if (!code.trim()) {
        logRun('Error: C++ editor is empty. Cannot run.', 'error');
        return;
    }

    btnRunCpp.classList.add('loading');
    logRun('Compiling and running C++ program...', 'system');

    try {
        const response = await fetch('/api/run', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ code })
        });

        const data = await response.json();
        if (data.success) {
            logRun('Compilation Succeeded! Execution Output:', 'success');
            logRun('--------------------------------------------------', 'system');
            
            const hasStdout = data.stdout && data.stdout.length > 0;
            const hasStderr = data.stderr && data.stderr.length > 0;
            
            if (hasStdout) {
                const lines = data.stdout.split(/\r?\n/);
                if (lines.length > 1 && lines[lines.length - 1] === '') {
                    lines.pop();
                }
                lines.forEach(ln => { logRun(ln, 'success'); });
            }
            
            if (hasStderr) {
                const lines = data.stderr.split(/\r?\n/);
                if (lines.length > 1 && lines[lines.length - 1] === '') {
                    lines.pop();
                }
                lines.forEach(ln => { logRun(ln, 'warning'); });
            }
            
            if (!hasStdout && !hasStderr) {
                logRun('(No output was written to standard output)', 'system');
            }
            
            logRun(`Exit Code: ${data.exitCode}`, 'system');
            logRun('--------------------------------------------------', 'system');
        } else {
            logRun('Compilation Failed! Compiler Error:', 'error');
            logRun('--------------------------------------------------', 'system');
            if (data.compileError) {
                const lines = data.compileError.split('\n');
                lines.forEach(ln => { if (ln.trim()) logRun(ln, 'error'); });
            }
            logRun('--------------------------------------------------', 'system');
        }
    } catch (err) {
        logRun(`Network Error: ${err.message}`, 'error');
    } finally {
        btnRunCpp.classList.remove('loading');
    }
});

// Add tab indentation support for textareas
function enableTabIndentation(textarea) {
    textarea.addEventListener('keydown', function(e) {
        if (e.key === 'Tab') {
            e.preventDefault();
            const start = this.selectionStart;
            const end = this.selectionEnd;

            // set textarea value to: text before caret + tab + text after caret
            this.value = this.value.substring(0, start) + "    " + this.value.substring(end);

            // put caret at right position
            this.selectionStart = this.selectionEnd = start + 4;
        }
    });
}

enableTabIndentation(cppEditor);
enableTabIndentation(asmEditor);

// Docs Modal Elements
const btnDocs = document.getElementById('btn-docs');
const docsModal = document.getElementById('docs-modal');
const closeModal = document.getElementById('close-modal');
const tabScope = document.getElementById('tab-scope');
const tabFaq = document.getElementById('tab-faq');
const contentScope = document.getElementById('content-scope');
const contentFaq = document.getElementById('content-faq');

// Open Modal
btnDocs.addEventListener('click', () => {
    docsModal.classList.add('active');
});

// Close Modal
closeModal.addEventListener('click', () => {
    docsModal.classList.remove('active');
});

// Close Modal clicking outside
docsModal.addEventListener('click', (e) => {
    if (e.target === docsModal) {
        docsModal.classList.remove('active');
    }
});

// Toggle Tabs
tabScope.addEventListener('click', () => {
    tabScope.classList.add('active');
    tabFaq.classList.remove('active');
    contentScope.classList.remove('hidden');
    contentFaq.classList.add('hidden');
});

tabFaq.addEventListener('click', () => {
    tabFaq.classList.add('active');
    tabScope.classList.remove('active');
    contentFaq.classList.remove('hidden');
    contentScope.classList.add('hidden');
});

// Tutorial Modal Elements
const tutorialModal = document.getElementById('tutorial-modal');
const btnTutorial = document.getElementById('btn-tutorial');
const closeTutorial = document.getElementById('close-tutorial');
const btnTutPrev = document.getElementById('btn-tut-prev');
const btnTutNext = document.getElementById('btn-tut-next');
const btnTutStart = document.getElementById('btn-tut-start');
const dots = document.querySelectorAll('.slide-dots .dot');
const slides = document.querySelectorAll('.t-slide');

let currentSlide = 1;
const totalSlides = 3;

function showSlide(slideNum) {
    slides.forEach(slide => slide.classList.remove('active'));
    dots.forEach(dot => dot.classList.remove('active'));
    
    document.getElementById(`t-slide-${slideNum}`).classList.add('active');
    document.querySelector(`.dot[data-slide="${slideNum}"]`).classList.add('active');
    
    currentSlide = slideNum;
    
    // Toggle buttons visibility
    if (currentSlide === 1) {
        btnTutPrev.style.display = 'none';
        btnTutNext.style.display = 'inline-block';
        btnTutStart.style.display = 'none';
    } else if (currentSlide === totalSlides) {
        btnTutPrev.style.display = 'inline-block';
        btnTutNext.style.display = 'none';
        btnTutStart.style.display = 'inline-block';
    } else {
        btnTutPrev.style.display = 'inline-block';
        btnTutNext.style.display = 'inline-block';
        btnTutStart.style.display = 'none';
    }
}

// Next/Prev Buttons
btnTutNext.addEventListener('click', () => {
    if (currentSlide < totalSlides) {
        showSlide(currentSlide + 1);
    }
});

btnTutPrev.addEventListener('click', () => {
    if (currentSlide > 1) {
        showSlide(currentSlide - 1);
    }
});

// Start Coding / Close Actions
const endTutorial = () => {
    tutorialModal.classList.remove('active');
    localStorage.setItem('bitranspiler_tutorial_seen', 'true');
};

btnTutStart.addEventListener('click', endTutorial);
closeTutorial.addEventListener('click', endTutorial);
tutorialModal.addEventListener('click', (e) => {
    if (e.target === tutorialModal) {
        endTutorial();
    }
});

// Dot Clicks
dots.forEach(dot => {
    dot.addEventListener('click', (e) => {
        const targetSlide = parseInt(e.target.getAttribute('data-slide'));
        showSlide(targetSlide);
    });
});

// Manual Re-open Tutorial from Header
btnTutorial.addEventListener('click', () => {
    showSlide(1);
    tutorialModal.classList.add('active');
});

// Auto-open on First Load
window.addEventListener('DOMContentLoaded', () => {
    const hasSeen = localStorage.getItem('bitranspiler_tutorial_seen');
    if (!hasSeen) {
        showSlide(1);
        tutorialModal.classList.add('active');
    }
});
