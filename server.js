const http = require('http');
const fs = require('fs');
const path = require('path');
const { spawn } = require('child_process');

const PORT = 3000;
const PUBLIC_DIR = path.join(__dirname, 'public');

// Append Winget GCC path to dynamically find dlls in child processes
const GCC_BIN_DIR = "C:\\Users\\RBTG V2\\AppData\\Local\\Microsoft\\WinGet\\Packages\\BrechtSanders.WinLibs.MCF.UCRT_Microsoft.Winget.Source_8wekyb3d8bbwe\\mingw64\\bin";

const server = http.createServer((req, res) => {
    // 1. Static Files Serving
    if (req.method === 'GET') {
        let filePath = req.url === '/' ? 'index.html' : req.url.slice(1);
        let absolutePath = path.join(PUBLIC_DIR, filePath);

        // Security check to prevent directory traversal
        if (!absolutePath.startsWith(PUBLIC_DIR)) {
            res.writeHead(403, { 'Content-Type': 'text/plain' });
            res.end('Forbidden');
            return;
        }

        fs.readFile(absolutePath, (err, data) => {
            if (err) {
                res.writeHead(404, { 'Content-Type': 'text/plain' });
                res.end('Not Found');
                return;
            }

            let contentType = 'text/plain';
            if (filePath.endsWith('.html')) contentType = 'text/html';
            else if (filePath.endsWith('.css')) contentType = 'text/css';
            else if (filePath.endsWith('.js')) contentType = 'application/javascript';

            res.writeHead(200, { 'Content-Type': contentType });
            res.end(data);
        });
        return;
    }

    // 2. Transpile API Endpoint
    if (req.method === 'POST' && req.url === '/api/transpile') {
        let body = '';
        req.on('data', chunk => { body += chunk; });
        req.on('end', () => {
            try {
                const payload = JSON.parse(body);
                const code = payload.code || '';
                const direction = payload.direction === 'cpp2asm' ? '--cpp2asm' : '--asm2cpp';

                const exePath = path.join(__dirname, 'BiTranspiler.exe');
                
                // Set path env so GCC DLLs are found
                const childEnv = { ...process.env };
                childEnv.PATH = (childEnv.PATH || '') + ';' + GCC_BIN_DIR;

                const child = spawn(exePath, [direction], { env: childEnv });

                let stdoutData = '';
                let stderrData = '';

                child.stdout.on('data', data => { stdoutData += data; });
                child.stderr.on('data', data => { stderrData += data; });

                child.on('error', err => {
                    res.writeHead(500, { 'Content-Type': 'application/json' });
                    res.end(JSON.stringify({ error: 'Failed to start transpiler process: ' + err.message }));
                });

                child.on('close', codeExit => {
                    if (codeExit === 0) {
                        res.writeHead(200, { 'Content-Type': 'application/json' });
                        res.end(JSON.stringify({
                            success: true,
                            result: stdoutData,
                            warnings: stderrData
                        }));
                    } else {
                        res.writeHead(400, { 'Content-Type': 'application/json' });
                        res.end(JSON.stringify({
                            success: false,
                            error: stderrData || 'Transpilation process exited with error code ' + codeExit
                        }));
                    }
                });

                // Write source code to stdin
                child.stdin.write(code);
                child.stdin.end();

            } catch (err) {
                res.writeHead(400, { 'Content-Type': 'application/json' });
                res.end(JSON.stringify({ error: 'Invalid JSON request payload' }));
            }
        });
        return;
    }

    // 3. Run C++ API Endpoint
    if (req.method === 'POST' && req.url === '/api/run') {
        let body = '';
        req.on('data', chunk => { body += chunk; });
        req.on('end', () => {
            try {
                const payload = JSON.parse(body);
                const code = payload.code || '';

                if (!code.trim()) {
                    res.writeHead(400, { 'Content-Type': 'application/json' });
                    res.end(JSON.stringify({ error: 'Code is empty' }));
                    return;
                }

                // Write code to a temp file
                const tempCpp = path.join(__dirname, 'temp_run.cpp');
                const tempExe = path.join(__dirname, 'temp_run.exe');

                // Clean up previous files if they exist
                if (fs.existsSync(tempCpp)) fs.unlinkSync(tempCpp);
                if (fs.existsSync(tempExe)) fs.unlinkSync(tempExe);

                fs.writeFile(tempCpp, code, (err) => {
                    if (err) {
                        res.writeHead(500, { 'Content-Type': 'application/json' });
                        res.end(JSON.stringify({ error: 'Failed to write temp source file' }));
                        return;
                    }

                    // Compile with g++
                    const compileEnv = { ...process.env };
                    compileEnv.PATH = (compileEnv.PATH || '') + ';' + GCC_BIN_DIR;

                    const compile = spawn('g++', ['-std=c++17', tempCpp, '-o', tempExe], { env: compileEnv });
                    let compileErr = '';

                    compile.stderr.on('data', data => { compileErr += data; });

                    compile.on('close', compileCode => {
                        // Delete temp source file
                        fs.unlink(tempCpp, () => {});

                        if (compileCode !== 0) {
                            res.writeHead(200, { 'Content-Type': 'application/json' });
                            res.end(JSON.stringify({
                                success: false,
                                compileError: compileErr || 'Compilation failed with exit code ' + compileCode
                            }));
                            return;
                        }

                        // Run executable
                        const runProcess = spawn(tempExe, [], { env: compileEnv });
                        let stdoutData = '';
                        let stderrData = '';

                        runProcess.stdout.on('data', data => { stdoutData += data; });
                        runProcess.stderr.on('data', data => { stderrData += data; });

                        runProcess.on('close', runCode => {
                            // Delete executable file
                            fs.unlink(tempExe, () => {});

                            res.writeHead(200, { 'Content-Type': 'application/json' });
                            res.end(JSON.stringify({
                                success: true,
                                stdout: stdoutData,
                                stderr: stderrData,
                                exitCode: runCode
                            }));
                        });
                    });
                });

            } catch (err) {
                res.writeHead(400, { 'Content-Type': 'application/json' });
                res.end(JSON.stringify({ error: 'Invalid JSON request payload' }));
            }
        });
        return;
    }

    // 4. Fallback 405
    res.writeHead(405, { 'Content-Type': 'text/plain' });
    res.end('Method Not Allowed');
});

server.listen(PORT, () => {
    console.log(`[SUCCESS] BiTranspiler live web GUI running at http://localhost:${PORT}`);
});
