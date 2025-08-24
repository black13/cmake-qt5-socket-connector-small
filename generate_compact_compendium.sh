#!/bin/bash

# Compact NodeGraph Code Compendium Generator
# Focuses on core functionality only

OUTPUT_DIR="compact_compendium"
MAIN_FILE="$OUTPUT_DIR/compact_code_review.txt"

echo "=== NodeGraph Compact Code Compendium Generator ==="
echo "Output directory: $OUTPUT_DIR"

# Create output directory
mkdir -p "$OUTPUT_DIR"

# Start main compendium file
echo "=== NodeGraph Compact Code Review Package ===" > "$MAIN_FILE"
echo "Generated: $(date)" >> "$MAIN_FILE"
echo "" >> "$MAIN_FILE"
echo "This is a focused code review package containing only the core functionality." >> "$MAIN_FILE"
echo "" >> "$MAIN_FILE"

# Function to add file with header
add_file() {
    local file="$1"
    if [[ -f "$file" ]]; then
        echo "Adding: $file"
        echo "=== $file ===" >> "$MAIN_FILE"
        echo "" >> "$MAIN_FILE"
        cat "$file" >> "$MAIN_FILE"
        echo "" >> "$MAIN_FILE"
        echo "" >> "$MAIN_FILE"
    else
        echo "WARNING: File not found: $file"
    fi
}

echo "Adding core application architecture..."

# Essential application files only
add_file "main.cpp"
add_file "window.h"
add_file "window.cpp"

echo "Adding scene and node system..."
add_file "scene.h" 
add_file "scene.cpp"
add_file "node.h"
add_file "node.cpp"
add_file "socket.h"
add_file "socket.cpp"
add_file "edge.h"
add_file "edge.cpp"

echo "Adding JavaScript integration..."
add_file "javascript_engine.h"
add_file "javascript_engine.cpp"
add_file "graph_controller.h"
add_file "graph_controller.cpp"

echo "Adding build system..."
add_file "CMakelists.txt"

# Create a compact README
echo "Creating compact README..."
cat > "$OUTPUT_DIR/README_COMPACT.md" << 'EOF'
# NodeGraph Compact Code Review

This is a focused code review package containing only the essential core files.

## Core Files Included

### Application Architecture
- `main.cpp` - Application entry point and initialization
- `window.h/cpp` - Main window and UI management

### Node Graph System  
- `scene.h/cpp` - Graphics scene managing nodes and edges
- `node.h/cpp` - Node implementation with socket management
- `socket.h/cpp` - Socket connection system
- `edge.h/cpp` - Edge connections between nodes

### JavaScript Integration
- `javascript_engine.h/cpp` - JavaScript engine for testing
- `graph_controller.h/cpp` - C++ to JavaScript bridge

### Build System
- `CMakelists.txt` - CMake build configuration

## Key Features Demonstrated

1. **Professional Qt5 Architecture** - Clean separation of concerns
2. **Socket Connection System** - Right-click drag functionality
3. **JavaScript Integration** - C++ to JS bridge for testing
4. **XML Serialization** - Proper socket count persistence
5. **Clean Code** - Professional logging without visual noise

## Total: ~8,000 lines of focused, production-ready code

This compact package demonstrates the core architecture and key innovations 
without overwhelming reviewers with auxiliary files.
EOF

# Copy key scripts
echo "Copying essential build script..."
cp "build.sh" "$OUTPUT_DIR/"

echo ""
echo "=== Compact Compendium Generation Complete ==="
echo "Output directory: $OUTPUT_DIR"
echo "Main file: $MAIN_FILE"

# Show statistics
if [[ -f "$MAIN_FILE" ]]; then
    LINES=$(wc -l < "$MAIN_FILE")
    SIZE=$(du -h "$MAIN_FILE" | cut -f1)
    echo "Total lines: $LINES"
    echo "File size: $SIZE"
fi

echo ""
echo "Files included:"
echo "- main.cpp (application entry point)"
echo "- window.h/cpp (main window)"
echo "- scene.h/cpp (graphics scene)"
echo "- node.h/cpp (node system)"
echo "- socket.h/cpp (connection system)" 
echo "- edge.h/cpp (edge connections)"
echo "- javascript_engine.h/cpp (JS integration)"
echo "- graph_controller.h/cpp (JS bridge)"
echo "- CMakelists.txt (build system)"
echo "- build.sh (build script)"
echo ""
echo "Ready for compact code review!"