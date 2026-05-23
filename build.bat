@echo off
title BiTranspiler Builder
echo ===================================================
echo             BiTranspiler Build System              
echo ===================================================
echo.

:: 1. Check if cl (MSVC) is already available in the current environment
where cl >nul 2>nul
if %ERRORLEVEL% equ 0 (
    echo [INFO] MSVC compiler found in PATH.
    goto :BUILD_MSVC
)

:: 2. Try to locate vcvars32.bat to set up MSVC environment
echo [INFO] Searching for Visual Studio 2022...

set "VS_PATH="
if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars32.bat" (
    set "VS_PATH=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars32.bat"
) else if exist "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars32.bat" (
    set "VS_PATH=C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars32.bat"
) else if exist "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars32.bat" (
    set "VS_PATH=C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars32.bat"
) else if exist "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars32.bat" (
    set "VS_PATH=C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars32.bat"
)

if not "%VS_PATH%"=="" (
    echo [INFO] Found Visual Studio variables at: "%VS_PATH%"
    echo [INFO] Setting up compiler environment...
    call "%VS_PATH%" >nul
    goto :BUILD_MSVC
)

:: 3. If MSVC not found, fall back to g++ (GCC via MinGW)
echo [WARN] MSVC (Visual Studio 2022) compiler not found or not initialized.
echo [INFO] Checking for g++ (GCC)...
where g++ >nul 2>nul
if %ERRORLEVEL% equ 0 (
    echo [INFO] g++ compiler found in PATH.
    goto :BUILD_GCC
)

:: Check for winget installed GCC
set "WINGET_GCC_DIR=C:\Users\RBTG V2\AppData\Local\Microsoft\WinGet\Packages\BrechtSanders.WinLibs.MCF.UCRT_Microsoft.Winget.Source_8wekyb3d8bbwe\mingw64\bin"
if exist "%WINGET_GCC_DIR%\g++.exe" (
    echo [INFO] Found winget-installed GCC at "%WINGET_GCC_DIR%"
    set "PATH=%PATH%;%WINGET_GCC_DIR%"
    goto :BUILD_GCC
)

echo [ERROR] No suitable C++ compiler found (cl.exe or g++).
echo Please install Visual Studio 2022 (with Desktop Development with C++ workloads) or MinGW (g++).
exit /b 1

:BUILD_MSVC
echo [BUILD] Compiling with MSVC...
cl /std:c++17 /EHsc /O2 /Fe:BiTranspiler.exe ^
    src/main.cpp ^
    src/lexer/CppLexer.cpp ^
    src/lexer/AsmLexer.cpp ^
    src/parser/CppParser.cpp ^
    src/parser/AsmParser.cpp ^
    src/codegen/AsmPrinter.cpp ^
    src/codegen/CppPrinter.cpp
if %ERRORLEVEL% equ 0 (
    echo.
    echo [SUCCESS] BiTranspiler.exe successfully built using MSVC!
    :: Clean up build objects
    del *.obj >nul 2>nul
    exit /b 0
) else (
    echo.
    echo [ERROR] Build failed.
    exit /b %ERRORLEVEL%
)

:BUILD_GCC
echo [BUILD] Compiling with g++...
g++ -std=c++17 -O2 -o BiTranspiler.exe ^
    src/main.cpp ^
    src/lexer/CppLexer.cpp ^
    src/lexer/AsmLexer.cpp ^
    src/parser/CppParser.cpp ^
    src/parser/AsmParser.cpp ^
    src/codegen/AsmPrinter.cpp ^
    src/codegen/CppPrinter.cpp
if %ERRORLEVEL% equ 0 (
    echo.
    echo [SUCCESS] BiTranspiler.exe successfully built using GCC!
    exit /b 0
) else (
    echo.
    echo [ERROR] Build failed.
    exit /b %ERRORLEVEL%
)
