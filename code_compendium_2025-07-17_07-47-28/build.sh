#!/bin/bash

# NodeGraph Linux Build Script for WSL
# Modern CMake build with robust configuration

set -e  # Exit on any error

echo "🚀 NodeGraph Linux Build Script"
echo "================================"

# Parse build type argument
BUILD_TYPE="Debug"
if [ "$1" = "release" ] || [ "$1" = "Release" ]; then
    BUILD_TYPE="Release"
elif [ "$1" = "debug" ] || [ "$1" = "Debug" ]; then
    BUILD_TYPE="Debug"
elif [ -n "$1" ]; then
    echo "Usage: $0 [debug|release]"
    echo "Default: debug"
    exit 1
fi

echo "Build Type: $BUILD_TYPE"

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

# Check if libxml2-dev is available
if ! pkg-config --exists libxml-2.0; then
    print_warning "libxml2-dev not found. Installing dependencies..."
    echo "Please run the following commands in another terminal:"
    echo "sudo apt update"
    echo "sudo apt install -y libxml2-dev build-essential"
    echo ""
    read -p "Press Enter after installing dependencies..."
    
    # Verify installation
    if ! pkg-config --exists libxml-2.0; then
        print_error "libxml2-dev still not found. Please install manually."
        exit 1
    fi
fi

print_success "libxml2-dev found: $(pkg-config --modversion libxml-2.0)"

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

QT_CMAKE_PATH="/usr/local/lib/cmake/Qt5"
if [ ! -f "$QT_CMAKE_PATH/Qt5Config.cmake" ]; then
    print_error "Qt5 not found in /usr/local. Please check Qt5 installation."
    exit 1
fi

print_success "Qt5 found in /usr/local"

# 3. Set up build environment
print_status "Setting up build environment..."

# Set Qt5 path for CMake
export CMAKE_PREFIX_PATH="/usr/local:$CMAKE_PREFIX_PATH"
export PKG_CONFIG_PATH="/usr/local/lib/pkgconfig:$PKG_CONFIG_PATH"

# Create build directory
BUILD_DIR="build_linux"
if [ -d "$BUILD_DIR" ]; then
    print_warning "Removing existing build directory..."
    rm -rf "$BUILD_DIR"
fi

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

print_success "Build directory created: $BUILD_DIR"

# 4. Configure with CMake
print_status "Configuring project with CMake..."

cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
      -DCMAKE_PREFIX_PATH="/usr/local" \
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
echo "Build type: Debug"
echo "Qt5 version: $(qmake -version | grep Qt | cut -d' ' -f4)"
echo "libxml2 version: $(pkg-config --modversion libxml-2.0)"
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

print_success "🎉 Build script completed successfully!"