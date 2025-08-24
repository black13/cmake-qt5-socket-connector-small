#!/bin/bash
# NodeGraph Runner Script for WSL
# Sets up Qt5 environment and runs the application

# Set Qt5 library path
export LD_LIBRARY_PATH="/usr/local/qt5.15.17-debug/lib:$LD_LIBRARY_PATH"

# Set display for X11 (if using X server)
export DISPLAY=:0

# Change to build directory
cd "$(dirname "$0")/build_linux"

echo "🚀 Starting NodeGraph with Qt5 Debug libraries..."
echo "   Qt5 Path: /usr/local/qt5.15.17-debug"
echo "   Display: $DISPLAY"
echo "   Working Directory: $(pwd)"
echo ""

# Run the application
./NodeGraph "$@"