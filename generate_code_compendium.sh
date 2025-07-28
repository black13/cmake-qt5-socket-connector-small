#!/bin/bash

# Code Compendium Generator
# Generates a comprehensive code review package for delivery
# This script is NOT part of the translation unit - it's for documentation/review only

set -e

# Allow specifying output directory or use default
OUTPUT_DIR="${1:-code_compendium}"
OUTPUT_FILE="${OUTPUT_DIR}/complete_code_compendium.txt"

echo "=== NodeGraph Code Compendium Generator ==="
echo "Output directory: ${OUTPUT_DIR}"
echo "Note: Use './generate_code_compendium.sh [directory_name]' to specify custom directory"

# Create output directory
mkdir -p "${OUTPUT_DIR}"

# Header for the compendium
cat > "${OUTPUT_FILE}" << 'EOF'
================================================================================
                      NodeGraph Code Compendium
                    Complete Source Code Review Package
================================================================================

Generated on: $(date)
Repository: cmake-qt5-socket-connector-small
Branch: $(git branch --show-current)
Commit: $(git rev-parse HEAD)

This compendium contains all source files for code review purposes.
NOT intended for compilation - this is a documentation artifact only.

================================================================================

EOF

# Function to add a file to the compendium
add_file_to_compendium() {
    local file="$1"
    local description="$2"
    
    if [[ -f "$file" ]]; then
        echo "Adding: $file"
        cat >> "${OUTPUT_FILE}" << EOF

================================================================================
FILE: $file
DESC: $description
================================================================================

EOF
        cat "$file" >> "${OUTPUT_FILE}"
        echo "" >> "${OUTPUT_FILE}"
    else
        echo "WARNING: File not found: $file"
    fi
}

# Add project metadata
echo "Adding project metadata..."
add_file_to_compendium "CMakelists.txt" "CMake build configuration"
add_file_to_compendium "README.md" "Project documentation (if exists)"
add_file_to_compendium "LICENSE" "Project license"

# Add core application files
echo "Adding core application files..."
add_file_to_compendium "main.cpp" "Application entry point"
add_file_to_compendium "window.h" "Main window header"
add_file_to_compendium "window.cpp" "Main window implementation"

# Add scene and view system
echo "Adding scene and view system..."
add_file_to_compendium "scene.h" "Graphics scene header"
add_file_to_compendium "scene.cpp" "Graphics scene implementation (includes ghost edge system)"
add_file_to_compendium "view.h" "Graphics view header"
add_file_to_compendium "view.cpp" "Graphics view implementation"

# Add node system
echo "Adding node system..."
add_file_to_compendium "node.h" "Node class header"
add_file_to_compendium "node.cpp" "Node class implementation"
add_file_to_compendium "socket.h" "Socket class header"
add_file_to_compendium "socket.cpp" "Socket class implementation (includes right-click drag)"
add_file_to_compendium "edge.h" "Edge class header"
add_file_to_compendium "edge.cpp" "Edge class implementation"

# Add graph system
echo "Adding graph system..."
add_file_to_compendium "graph_factory.h" "Graph factory header"
add_file_to_compendium "graph_factory.cpp" "Graph factory implementation"
add_file_to_compendium "node_registry.h" "Node registry header"
add_file_to_compendium "node_registry.cpp" "Node registry implementation"

# Add XML and persistence
echo "Adding XML and persistence..."
add_file_to_compendium "xml_autosave_observer.h" "XML autosave observer header"
add_file_to_compendium "xml_autosave_observer.cpp" "XML autosave observer implementation"
add_file_to_compendium "graph_observer.h" "Graph observer header"
add_file_to_compendium "graph_observer.cpp" "Graph observer implementation"

# Add JavaScript integration
echo "Adding JavaScript integration..."
add_file_to_compendium "javascript_engine.h" "JavaScript engine header"
add_file_to_compendium "javascript_engine.cpp" "JavaScript engine implementation"
add_file_to_compendium "graph_controller.h" "Graph controller header for JS"
add_file_to_compendium "graph_controller.cpp" "Graph controller implementation for JS"

# Add test files
echo "Adding test files..."
add_file_to_compendium "tst_main.cpp" "Main test file"
add_file_to_compendium "tst_main.h" "Main test header"

# Add build scripts
echo "Adding build scripts..."
add_file_to_compendium "build.sh" "Linux build script"
add_file_to_compendium "build.bat" "Windows build script"

# Add resource files
echo "Adding resource files..."
add_file_to_compendium "icons.qrc" "Qt resource file"

# Add any additional implementation files that might exist
for file in *.h *.cpp; do
    if [[ -f "$file" ]] && ! grep -q "FILE: $file" "${OUTPUT_FILE}"; then
        echo "Adding additional file: $file"
        add_file_to_compendium "$file" "Additional implementation file"
    fi
done

# Add project documentation files
echo "Adding project documentation..."
add_file_to_compendium "log.md" "Development conversation log"
add_file_to_compendium "GRAPH_SPECIFICATION.md" "Graph specification (if exists)"
add_file_to_compendium "PERFORMANCE_OPTIMIZATION.md" "Performance notes (if exists)"

# Add footer
cat >> "${OUTPUT_FILE}" << 'EOF'

================================================================================
                           END OF CODE COMPENDIUM
================================================================================

This compendium was generated automatically for code review purposes.
It contains all source files needed to understand the complete implementation.

Key Features Implemented:
- Right-click socket drag connections with ghost edge visual feedback
- Self-serializing XML-first architecture
- Observer pattern for automatic persistence
- JavaScript integration for testing and automation
- Professional Qt5 GUI with status bar and menus
- Clean startup without hardcoded test nodes

Build Instructions:
- Linux/WSL: Run ./build.sh
- Windows: Run build.bat
- Requires Qt5, libxml2, and standard C++ compiler

For questions about this code, refer to log.md for implementation details.

EOF

# Generate file statistics
echo "Generating statistics..."
cat >> "${OUTPUT_FILE}" << EOF

================================================================================
                              FILE STATISTICS
================================================================================

Total files included: $(grep -c "FILE:" "${OUTPUT_FILE}")
Total lines of code: $(wc -l < "${OUTPUT_FILE}")
Generated on: $(date)
Repository state: $(git describe --always --dirty)

EOF

# Copy important configuration files to output directory
echo "Copying additional files..."
cp -f CMakelists.txt "${OUTPUT_DIR}/" 2>/dev/null || true
cp -f build.sh "${OUTPUT_DIR}/" 2>/dev/null || true
cp -f build.bat "${OUTPUT_DIR}/" 2>/dev/null || true
cp -f log.md "${OUTPUT_DIR}/" 2>/dev/null || true

# Create a README for the compendium
cat > "${OUTPUT_DIR}/README_COMPENDIUM.md" << 'EOF'
# NodeGraph Code Compendium

This directory contains a complete code review package for the NodeGraph project.

## Contents

- `complete_code_compendium.txt` - All source files concatenated for review
- `CMakelists.txt` - Build configuration
- `build.sh` / `build.bat` - Build scripts
- `log.md` - Development conversation log
- `README_COMPENDIUM.md` - This file

## Usage

This compendium is intended for code review and documentation purposes only.
It is NOT a buildable package - use the original repository for building.

## Key Features

- Right-click socket drag connections
- Ghost edge visual feedback system
- XML-first self-serializing architecture
- JavaScript integration for testing
- Professional Qt5 GUI

## Review Focus Areas

1. Socket connection system (socket.cpp, scene.cpp)
2. Ghost edge visual feedback (scene.cpp)
3. XML serialization architecture (graph_factory.cpp, xml_autosave_observer.cpp)
4. JavaScript integration (javascript_engine.cpp, graph_controller.cpp)
5. Clean startup implementation (main.cpp, window.cpp)

EOF

echo ""
echo "=== Code Compendium Generation Complete ==="
echo "Output directory: ${OUTPUT_DIR}"
echo "Main compendium file: ${OUTPUT_FILE}"
echo "Total size: $(du -sh "${OUTPUT_DIR}" | cut -f1)"
echo ""
echo "Files included:"
grep "FILE:" "${OUTPUT_FILE}" | sed 's/FILE: /- /'
echo ""
echo "Ready for reviewer delivery!"