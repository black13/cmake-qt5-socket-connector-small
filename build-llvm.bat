@echo off
REM =============================================================
REM  build-llvm.bat - configure + build NodeGraph with Clang/LLVM
REM                    + source-based code coverage instrumentation
REM
REM  Usage: build-llvm.bat [debug|release] [clean]
REM  • build-llvm.bat          - builds debug (default)
REM  • build-llvm.bat debug    - debug build with coverage instrumentation
REM  • build-llvm.bat release  - release build (coverage still on)
REM  • build-llvm.bat debug clean - full clean rebuild
REM
REM  Prerequisites:
REM  • LLVM 22.1.x installed at C:\Program Files\LLVM
REM  • Ninja installed (winget install Ninja-build.Ninja)
REM  • Run inside a "Developer Command Prompt for VS 2022"
REM    (clang-cl needs MSVC headers + libraries)
REM  • Qt 5.15.x at D:\Qt\5.15.19\debug and D:\Qt\5.15.19\release
REM
REM  Coverage workflow:
REM  1. Build:       build-llvm.bat debug
REM  2. Set env:     set LLVM_PROFILE_FILE=nodegraph_%%p_%%m.profraw
REM  3. Run:         build_llvm\NodeGraph.exe
REM  4. Merge:       llvm-profdata merge -sparse *.profraw -o merged.profdata
REM  5. Report:      llvm-cov show build_llvm\NodeGraph.exe -instr-profile=merged.profdata
REM  6. HTML:        llvm-cov show build_llvm\NodeGraph.exe -instr-profile=merged.profdata -format=html -output-dir=coverage_report
REM =============================================================

REM -------- 1: goto repo root and parse arguments
cd /d "%~dp0"
set BUILD_TYPE=%1
set CLEAN_BUILD=%2
if "%BUILD_TYPE%"=="" set BUILD_TYPE=debug

REM -------- 2: Detect LLVM install path
set LLVM_PATH=C:\Program Files\LLVM
if not exist "%LLVM_PATH%\bin\clang-cl.exe" (
    echo *** LLVM not found at %LLVM_PATH%
    echo *** Install via: winget install LLVM.LLVM --version 22.1.8
    pause
    exit /b 1
)

REM -------- 3: Verify Ninja is available
where ninja >nul 2>&1
if errorlevel 1 (
    echo *** Ninja not found in PATH
    echo *** Install via: winget install Ninja-build.Ninja
    echo *** Then restart your shell
    pause
    exit /b 1
)

REM -------- 4: Verify VS environment (clang-cl needs MSVC headers/libs)
where cl.exe >nul 2>&1
if errorlevel 1 (
    echo *** MSVC compiler (cl.exe) not found in PATH
    echo *** Run this script from a "Developer Command Prompt for VS 2022"
    echo ***   Start menu -> Visual Studio 2022 -> Developer Command Prompt
    echo *** Or: x64 Native Tools Command Prompt for VS 2022
    pause
    exit /b 1
)

REM -------- 5: Set Qt5 paths and build directory
if /i "%BUILD_TYPE%"=="release" (
    set QT5_PATH=D:\Qt\5.15.19\release
    set BUILD_DIR=build_llvm_Release
    set CMAKE_BUILD_CFG=Release
    echo === Using Qt5 RELEASE libraries ===
) else (
    set QT5_PATH=D:\Qt\5.15.19\debug
    set BUILD_DIR=build_llvm
    set CMAKE_BUILD_CFG=Debug
    echo === Using Qt5 DEBUG libraries ===
)

set CMAKE_PREFIX_PATH=%QT5_PATH%\lib\cmake
set PATH=%LLVM_PATH%\bin;%QT5_PATH%\bin;%PATH%

echo ============ LLVM BUILD ENVIRONMENT ============
echo Build Type:       %BUILD_TYPE%
echo C Compiler:       %LLVM_PATH%\bin\clang-cl.exe
echo CXX Compiler:     %LLVM_PATH%\bin\clang-cl.exe
echo Qt5 Path:         %QT5_PATH%
echo Build Directory:  %BUILD_DIR%
echo CMAKE_PREFIX_PATH:%CMAKE_PREFIX_PATH%
echo Coverage:         ENABLED (source-based)
echo Profraw pattern:  nodegraph_%%p_%%m.profraw
echo ===============================================
echo.

REM -------- 6: Verify clang-cl works
"%LLVM_PATH%\bin\clang-cl.exe" --version 2>&1 | findstr /C:"clang version" >nul
if errorlevel 1 (
    echo *** Failed to run clang-cl.exe
    pause
    exit /b 1
)

REM -------- 7: Handle build directory (clean vs cached)
if /i "%CLEAN_BUILD%"=="clean" (
    echo === FULL CLEAN: Removing %BUILD_DIR% directory ===
    if exist %BUILD_DIR% rmdir /s /q %BUILD_DIR%
) else (
    if exist %BUILD_DIR% (
        echo === Build directory exists - cmake cache preserved ===
    )
)
if not exist %BUILD_DIR% mkdir %BUILD_DIR%

REM -------- 8: CMake configure with Ninja + clang-cl + coverage
echo === CMake configure (Ninja + clang-cl + coverage) ===
cmake -S . -B %BUILD_DIR% ^
      -G "Ninja" ^
      -DCMAKE_C_COMPILER="%LLVM_PATH%\bin\clang-cl.exe" ^
      -DCMAKE_CXX_COMPILER="%LLVM_PATH%\bin\clang-cl.exe" ^
      -DCMAKE_BUILD_TYPE=%CMAKE_BUILD_CFG% ^
      -DCMAKE_PREFIX_PATH="%CMAKE_PREFIX_PATH%" ^
      -DENABLE_LLVM_COVERAGE=ON

if errorlevel 1 (
    echo.
    echo *** CMake configuration FAILED ***
    echo.
    echo Troubleshooting:
    echo 1. Run from "Developer Command Prompt for VS 2022"
    echo 2. Verify Qt5 is installed at %QT5_PATH%
    echo 3. Check that LLVM is at %LLVM_PATH%
    echo 4. Check that Ninja is installed and in PATH
    echo 5. Try: winget install Ninja-build.Ninja
    echo.
    pause
    exit /b 1
)

REM -------- 9: Build with Ninja
echo === Building with Ninja (%CMAKE_BUILD_CFG%) ===
if /i "%CLEAN_BUILD%"=="clean" (
    cmake --build %BUILD_DIR% --clean-first
) else (
    cmake --build %BUILD_DIR%
)

if errorlevel 1 (
    echo.
    echo *** Build FAILED ***
    echo.
    echo Common issues:
    echo - libxml2 fails to build with clang-cl: try running without clean first
    echo   (FetchContent cache must be populated by the same compiler)
    echo - If switching from MSVC, delete .cmake-cache/ and build_llvm/ first
    echo.
    pause
    exit /b 1
)

echo.
echo ===============================================
echo * LLVM BUILD COMPLETE (coverage instrumented)
echo ===============================================
echo.
echo Executable: %BUILD_DIR%\NodeGraph.exe
echo.
echo --- Coverage Collection ---
echo 1. set LLVM_PROFILE_FILE=nodegraph_%%p_%%m.profraw
echo 2. %BUILD_DIR%\NodeGraph.exe [--script script.js]
echo 3. C:\Program Files\LLVM\bin\llvm-profdata.exe merge -sparse *.profraw -o merged.profdata
echo 4. C:\Program Files\LLVM\bin\llvm-cov.exe show %BUILD_DIR%\NodeGraph.exe -instr-profile=merged.profdata
echo.
echo HTML Report:
echo   C:\Program Files\LLVM\bin\llvm-cov.exe show %BUILD_DIR%\NodeGraph.exe -instr-profile=merged.profdata -format=html -output-dir=coverage_report
echo   Then open coverage_report\index.html
echo ===============================================
pause