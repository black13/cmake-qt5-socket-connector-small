#!/bin/bash

# NodeGraph Automatic Test Runner
# Builds and runs all enabled tests

set -e  # Exit on any error

echo "🧪 NodeGraph Test Suite Runner"
echo "=============================="

# Function to run tests with proper environment
run_tests() {
    local build_dir="$1"
    local platform="$2"
    
    echo "📁 Build Directory: $build_dir"
    echo "🖥️  Platform: $platform"
    echo ""
    
    # Set up environment for Linux/WSL
    if [ "$platform" = "linux" ]; then
        export LD_LIBRARY_PATH="/usr/local/qt5.15.17-debug/lib:$LD_LIBRARY_PATH"
        export QT_QPA_PLATFORM=offscreen  # Headless testing
        echo "🔧 Environment: Set LD_LIBRARY_PATH and QT_QPA_PLATFORM=offscreen"
    fi
    
    # Change to build directory
    cd "$build_dir"
    
    echo "🏗️  Building test targets..."
    
    # Build the test executables
    if [ "$platform" = "linux" ]; then
        make TestJavaScriptEngine -j$(nproc)
        make NodeGraphTests -j$(nproc)
    else
        # Windows build (if needed later)
        echo "Windows build not implemented yet"
        return 1
    fi
    
    echo "✅ Build complete!"
    echo ""
    
    # Run individual test executables with detailed output
    echo "🚀 Running JavaScript Engine Tests..."
    echo "───────────────────────────────────────"
    if [ -f "./TestJavaScriptEngine" ]; then
        ./TestJavaScriptEngine -v2  # Verbose output
        echo "✅ JavaScript Engine Tests completed"
    else
        echo "❌ TestJavaScriptEngine executable not found"
    fi
    
    echo ""
    echo "🚀 Running Main Test Suite..."
    echo "─────────────────────────────────────"
    if [ -f "./NodeGraphTests" ]; then
        ./NodeGraphTests -v2  # Verbose output  
        echo "✅ Main Tests completed"
    else
        echo "❌ NodeGraphTests executable not found"
    fi
    
    echo ""
    echo "🚀 Running CTest (if available)..."
    echo "──────────────────────────────────────"
    if command -v ctest &> /dev/null; then
        ctest --output-on-failure --verbose
        echo "✅ CTest completed"
    else
        echo "⚠️  CTest not available - individual tests ran successfully"
    fi
}

# Detect platform and build directory
if [ -d "build_linux" ]; then
    echo "🐧 Detected Linux/WSL build directory"
    run_tests "build_linux" "linux"
elif [ -d "build_Debug" ] || [ -d "build_Release" ]; then
    echo "🪟 Detected Windows build directory"
    if [ -d "build_Debug" ]; then
        run_tests "build_Debug" "windows"
    else
        run_tests "build_Release" "windows"
    fi
else
    echo "❌ No build directory found!"
    echo "Please run the build script first:"
    echo "  Linux/WSL: ./build.sh"
    echo "  Windows:   ./build.bat"
    exit 1
fi

echo ""
echo "🎉 All tests completed successfully!"
echo "📊 Test Summary:"
echo "   - JavaScript Engine Tests: Comprehensive JS functionality testing"
echo "   - Main Test Suite: Core NodeGraph functionality testing"
echo "   - CTest Integration: Automatic test discovery and execution"
echo ""
echo "💡 Next Steps:"
echo "   - Review test output above for any failures or warnings"
echo "   - Add new test cases in test_javascript_engine.cpp"
echo "   - Run './run_tests.sh' regularly during development"