@echo off
REM =============================================================
REM  build.bat - configure + build NodeGraph with MSBuild (/m)
REM
REM  Usage: build.bat [debug|release|both] [clean]
REM  • build.bat         - builds debug (default, preserves libxml2 cache)
REM  • build.bat debug   - builds debug for VS debugging (cached)
REM  • build.bat release - builds release for testing (cached)  
REM  • build.bat both    - builds both configurations (cached)
REM  • build.bat debug clean   - full clean debug build (rebuilds libxml2)
REM  • build.bat release clean - full clean release build (rebuilds libxml2)
REM  • build.bat both clean    - full clean both builds (rebuilds libxml2)
REM
REM  • Run inside a "Developer Command Prompt for VS"
REM  • Qt 5.15.x's <bin> dir on PATH (qmake, moc, ...)
REM  • Cached builds are much faster (preserves libxml2 build cache)
REM =============================================================

REM -------- 1: goto repo root and parse arguments
cd /d "%~dp0"
set BUILD_TYPE=%1
set CLEAN_BUILD=%2
if "%BUILD_TYPE%"=="" set BUILD_TYPE=debug

REM -------- 2: Set Qt5 paths and build directories based on build type
if /i "%BUILD_TYPE%"=="both" goto BUILD_BOTH

if /i "%BUILD_TYPE%"=="release" (
    set QT5_PATH=D:\Qt-5.15.17-msvc142-x64-Release\msvc2019_64
    set BUILD_DIR=build_Release
    echo === Using Qt5 RELEASE libraries ===
) else (
    set QT5_PATH=D:\Qt-5.15.17-msvc142-x64-Debug\msvc2019_64
    set BUILD_DIR=build_Debug
    echo === Using Qt5 DEBUG libraries ===
)

set CMAKE_PREFIX_PATH=%QT5_PATH%\lib\cmake
set PATH=%QT5_PATH%\bin;%PATH%

echo ============ ENVIRONMENT DEBUG INFO ============
echo Build Type: %BUILD_TYPE%
echo Qt5 Path: %QT5_PATH%
echo Build Directory: %BUILD_DIR%
echo CMAKE_PREFIX_PATH: %CMAKE_PREFIX_PATH%
echo PATH (first 200 chars): %PATH:~0,200%...
echo Current Directory: %CD%
echo ===============================================
echo.

REM -------- 3: handle build directory (clean vs cached)
if /i "%CLEAN_BUILD%"=="clean" (
    echo === FULL CLEAN: Removing %BUILD_DIR% directory ===
    echo This will rebuild libxml2 from scratch - slower but fresh build
    if exist %BUILD_DIR% rmdir /s /q %BUILD_DIR%
) else (
    echo === CACHED BUILD: Preserving libxml2 cache ===
    if exist %BUILD_DIR% (
        echo Build directory exists - libxml2 cache will be preserved for faster build
    ) else (
        echo Build directory doesn't exist - libxml2 will be built once and cached
    )
)
if not exist %BUILD_DIR% mkdir %BUILD_DIR%

REM -------- 4: configure with modern CMake syntax
echo === CMake configure for %BUILD_TYPE% ===
if /i "%BUILD_TYPE%"=="release" (
    cmake -S . -B %BUILD_DIR% ^
          -G "Visual Studio 17 2022" ^
          -A x64 ^
          -DCMAKE_BUILD_TYPE=Release ^
          -DCMAKE_PREFIX_PATH="%CMAKE_PREFIX_PATH%"
) else (
    cmake -S . -B %BUILD_DIR% ^
          -G "Visual Studio 17 2022" ^
          -A x64 ^
          -DCMAKE_BUILD_TYPE=Debug ^
          -DCMAKE_PREFIX_PATH="%CMAKE_PREFIX_PATH%"
)

if errorlevel 1 (
    echo *** CMake configuration FAILED ***
    echo.
    echo Troubleshooting:
    echo 1. Make sure you're in a "Developer Command Prompt for VS 2022"
    echo 2. Verify Qt5 is installed at D:\Qt-5.15.17-msvc142-x64-Debug\msvc2019_64 and D:\Qt-5.15.17-msvc142-x64-Release\msvc2019_64
    echo 3. Check that cmake is in your PATH
    echo 4. Edit this script to match your Qt installation path
    echo.
    pause
    exit /b 1
)

REM -------- 5: build based on argument
if /i "%BUILD_TYPE%"=="release" goto BUILD_RELEASE
if /i "%BUILD_TYPE%"=="debug" goto BUILD_DEBUG
goto BUILD_DEBUG

:BUILD_RELEASE
echo === Building Release (MSBuild, /m) ===
if /i "%CLEAN_BUILD%"=="clean" (
    cmake --build %BUILD_DIR% --config Release --clean-first -- /m
) else (
    cmake --build %BUILD_DIR% --config Release -- /m
)
if errorlevel 1 (
    echo *** Release Build FAILED ***
    pause
    exit /b 1
)
echo * RELEASE build complete
echo * RELEASE build active - optimized for performance
echo * Solution file: %BUILD_DIR%\NodeGraph.sln
goto END

:BUILD_DEBUG
echo === Building Debug (MSBuild, /m) ===  
if /i "%CLEAN_BUILD%"=="clean" (
    cmake --build %BUILD_DIR% --config Debug --clean-first -- /m
) else (
    cmake --build %BUILD_DIR% --config Debug -- /m
)
if errorlevel 1 (
    echo *** Debug Build FAILED ***
    pause
    exit /b 1
)
echo * DEBUG build complete
echo * DEBUG build active - ready for VS debugging
echo * Solution file: %BUILD_DIR%\NodeGraph.sln
goto END

:BUILD_BOTH
echo === Building both Debug and Release configurations ===

REM Handle clean vs cached for both builds
if /i "%CLEAN_BUILD%"=="clean" (
    echo === FULL CLEAN: Removing both build directories ===
    echo This will rebuild libxml2 from scratch for both - slower but fresh builds
    if exist build_Debug rmdir /s /q build_Debug
    if exist build_Release rmdir /s /q build_Release
) else (
    echo === CACHED BUILDS: Preserving libxml2 cache for both ===
    if exist build_Debug (
        echo Debug build directory exists - libxml2 cache preserved
    )
    if exist build_Release (
        echo Release build directory exists - libxml2 cache preserved  
    )
)

REM Debug first
set QT5_PATH=D:\Qt-5.15.17-msvc142-x64-Debug\msvc2019_64
set BUILD_DIR=build_Debug
set CMAKE_PREFIX_PATH=%QT5_PATH%\lib\cmake
set PATH=%QT5_PATH%\bin;%PATH%

echo === Configuring DEBUG ===
if not exist %BUILD_DIR% mkdir %BUILD_DIR%
cmake -S . -B %BUILD_DIR% -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH="%CMAKE_PREFIX_PATH%"
if errorlevel 1 goto BUILD_ERROR

echo === Building DEBUG ===
if /i "%CLEAN_BUILD%"=="clean" (
    cmake --build %BUILD_DIR% --config Debug --clean-first -- /m
) else (
    cmake --build %BUILD_DIR% --config Debug -- /m
)
if errorlevel 1 goto BUILD_ERROR

REM Release second
set QT5_PATH=D:\Qt-5.15.17-msvc142-x64-Release\msvc2019_64
set BUILD_DIR=build_Release
set CMAKE_PREFIX_PATH=%QT5_PATH%\lib\cmake
set PATH=%QT5_PATH%\bin;%PATH%

echo === Configuring RELEASE ===
if not exist %BUILD_DIR% mkdir %BUILD_DIR%
cmake -S . -B %BUILD_DIR% -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="%CMAKE_PREFIX_PATH%"
if errorlevel 1 goto BUILD_ERROR

echo === Building RELEASE ===
if /i "%CLEAN_BUILD%"=="clean" (
    cmake --build %BUILD_DIR% --config Release --clean-first -- /m
) else (
    cmake --build %BUILD_DIR% --config Release -- /m
)
if errorlevel 1 goto BUILD_ERROR

echo * Both builds complete
echo * DEBUG build active for development
echo * Debug solution: build_Debug\NodeGraph.sln
echo * Release solution: build_Release\NodeGraph.sln
goto END

:BUILD_ERROR
echo *** Build FAILED ***
pause
exit /b 1

:END
echo.
echo ===============================================
if /i "%BUILD_TYPE%"=="both" (
    echo Debug executable: build_Debug\Debug\NodeGraph.exe
    echo Release executable: build_Release\Release\NodeGraph.exe
    echo Debug solution: build_Debug\NodeGraph.sln
    echo Release solution: build_Release\NodeGraph.sln
) else if /i "%BUILD_TYPE%"=="release" (
    echo Executable: build_Release\Release\NodeGraph.exe
    echo Solution file: build_Release\NodeGraph.sln
) else (
    echo Executable: build_Debug\Debug\NodeGraph.exe
    echo Solution file: build_Debug\NodeGraph.sln
)
echo ===============================================
echo.
echo * Build completed successfully!
echo * You can now run the application or open the solution in Visual Studio
pause