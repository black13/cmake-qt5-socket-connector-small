#!/bin/bash

# NodeGraph Linux Build Script for WSL
# Modern CMake build with robust configuration

set -e  # Exit on any error

echo "NodeGraph Linux Build Script"
echo "================================"

# Parse build type and clean arguments
BUILD_TYPE="Debug"
CLEAN_BUILD=false

# Parse all arguments
for arg in "$@"; do
    case $arg in
        release|Release)
            BUILD_TYPE="Release"
            ;;
        debug|Debug)
            BUILD_TYPE="Debug"
            ;;
        clean)
            CLEAN_BUILD=true
            ;;
        *)
            echo "Usage: $0 [debug|release] [clean]"
            echo "Examples:"
            echo "  $0 debug       # Debug build (incremental)"
            echo "  $0 release     # Release build (incremental)"
            echo "  $0 debug clean # Debug build (clean)"
            echo "  $0 clean debug # Debug build (clean)"
            echo "Default: debug (incremental)"
            exit 1
            ;;
    esac
done

echo "Build Type: $BUILD_TYPE"
if [ "$CLEAN_BUILD" = true ]; then
    echo "Clean Build: Yes"
else
    echo "Clean Build: No (incremental)"
fi

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Function to print colored output
print_status() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Check if we're in WSL
if grep -qi microsoft /proc/version; then
    print_status "Running in WSL environment"
else
    print_warning "Not detected as WSL - continuing anyway"
fi

# 1. Check and install dependencies
print_status "Checking dependencies..."

# Using FetchContent for libxml2 - no system dependencies required
print_success "Using FetchContent for libxml2 (no system dependencies needed)"

# Check build tools
for tool in cmake gcc g++ make; do
    if ! command -v $tool &> /dev/null; then
        print_error "$tool not found. Please install build-essential."
        exit 1
    fi
done

print_success "Build tools available"

# 2. Check Qt5 installation
print_status "Checking Qt5 installation..."

# Auto-detect Qt installations in /opt/qt* (preferred) or /usr/local/qt-*
QT_INSTALLS=($(find /opt /usr/local -maxdepth 1 -name "qt*" -type d 2>/dev/null | sort -V -r))

if [ ${#QT_INSTALLS[@]} -eq 0 ]; then
    print_error "No Qt installations found in /opt/qt* or /usr/local/qt-*"
    print_error "Please install Qt5 to /opt/qt-VERSION or /usr/local/qt-VERSION"
    exit 1
fi

print_status "Found Qt installations:"
for qt_dir in "${QT_INSTALLS[@]}"; do
    echo "  - $qt_dir"
done

QT_PATH=""

# Selection strategy: prefer generic install first, then -debug/-release
for qt_dir in "${QT_INSTALLS[@]}"; do
    if [[ "$qt_dir" != *"-debug"* && "$qt_dir" != *"-release"* ]] && [ -d "$qt_dir/lib/cmake/Qt5" ]; then
        QT_PATH="$qt_dir"
        print_success "Selected Qt5 installation: $QT_PATH"
        break
    fi
done

# If not found, fall back to build-type-specific directories
if [ -z "$QT_PATH" ]; then
    if [ "$BUILD_TYPE" = "Debug" ]; then
        for qt_dir in "${QT_INSTALLS[@]}"; do
            if [[ "$qt_dir" == *"-debug"* ]] && [ -d "$qt_dir/lib/cmake/Qt5" ]; then
                QT_PATH="$qt_dir"
                print_success "Selected Qt5 Debug build: $QT_PATH"
                break
            fi
        done
    else
        for qt_dir in "${QT_INSTALLS[@]}"; do
            if [[ "$qt_dir" == *"-release"* ]] && [ -d "$qt_dir/lib/cmake/Qt5" ]; then
                QT_PATH="$qt_dir"
                print_success "Selected Qt5 Release build: $QT_PATH"
                break
            fi
        done
    fi
fi

# Final fallback to any available Qt installation
if [ -z "$QT_PATH" ]; then
    for qt_dir in "${QT_INSTALLS[@]}"; do
        if [ -d "$qt_dir/lib/cmake/Qt5" ]; then
            QT_PATH="$qt_dir"
            print_success "Selected Qt5 installation: $QT_PATH"
            break
        fi
    done
fi

# Final check
if [ -z "$QT_PATH" ] || [ ! -d "$QT_PATH/lib/cmake/Qt5" ]; then
    print_error "No valid Qt5 installation found with CMake support"
    print_error "Make sure Qt5 is installed with development files"
    exit 1
fi

# 3. Set up build environment
print_status "Setting up build environment..."

# Set Qt5 path for CMake
export CMAKE_PREFIX_PATH="$QT_PATH:$CMAKE_PREFIX_PATH"  
export PKG_CONFIG_PATH="$QT_PATH/lib/pkgconfig:$PKG_CONFIG_PATH"

# Create build directory with smart cache preservation
BUILD_DIR="build_linux"
CACHE_DIR=".cmake-cache"

if [ -d "$BUILD_DIR" ] && [ "$CLEAN_BUILD" = true ]; then
    print_warning "Clean build requested - removing existing build directory..."
    rm -rf "$BUILD_DIR"
elif [ -d "$BUILD_DIR" ]; then
    print_status "Preserving existing build directory for incremental build"
fi

# Preserve cache directory even during clean builds
if [ -d "$CACHE_DIR" ]; then
    print_status "Preserving libxml2 cache directory: $CACHE_DIR"
else
    print_status "Cache directory will be created: $CACHE_DIR"
fi

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

print_success "Build directory ready: $BUILD_DIR"

# 4. Configure with CMake
print_status "Configuring project with CMake..."

cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
      -DCMAKE_PREFIX_PATH="$QT_PATH" \
      -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
      .. || {
    print_error "CMake configuration failed!"
    exit 1
}

print_success "CMake configuration completed"

# 5. Build the project
print_status "Building NodeGraph..."

# Use modern CMake build command with all available cores
CORES=$(nproc)
print_status "Building with $CORES cores..."

cmake --build . --config $BUILD_TYPE --parallel $CORES || {
    print_error "Build failed!"
    exit 1
}

print_success "Build completed successfully!"

# 6. Check if X11 is available for GUI
print_status "Checking X11 environment..."

if [ -z "$DISPLAY" ]; then
    print_warning "DISPLAY not set. You'll need X11 server on Windows."
    echo "To run the GUI application:"
    echo "1. Install VcXsrv or X410 on Windows"
    echo "2. Start X server with display :0"
    echo "3. Run: export DISPLAY=:0"
    echo "4. Then run: ./NodeGraph"
else
    print_success "DISPLAY set to: $DISPLAY"
fi

# 7. Show build results
print_status "Build Summary"
echo "=============="
echo "Executable: $(pwd)/NodeGraph"
echo "Build type: $BUILD_TYPE"
echo "Qt5 version: $(qmake -version | grep Qt | cut -d' ' -f4)"
echo "libxml2 source: FetchContent (built from source)"
echo ""

if [ -f "NodeGraph" ]; then
    print_success "NodeGraph executable created successfully!"
    echo ""
    print_status "To test the application:"
    echo "cd $(pwd)"
    echo "export DISPLAY=:0  # if using X11 server"
    echo "./NodeGraph"
else
    print_error "NodeGraph executable not found!"
    exit 1
fi

print_success "Build script completed successfully!"
