#!/bin/bash

# Short Compendium Generator - Translation Unit Only
# Creates a focused compendium of only the compiled source files

set -e

OUTPUT_DIR="${1:-short_compendium}"
OUTPUT_FILE="${OUTPUT_DIR}/compiled_source.txt"

echo "=== Short Translation Unit Compendium ==="
echo "Output: ${OUTPUT_DIR}"

mkdir -p "${OUTPUT_DIR}"

# Header
cat > "${OUTPUT_FILE}" << 'EOF'
================================================================================
                    NodeGraph - Compiled Source Code Only
================================================================================

Translation unit files that are compiled into the executable.
Generated: $(date)

================================================================================

EOF

# Function to add file
add_file() {
    local file="$1"
    if [[ -f "$file" ]]; then
        echo "Adding: $file"
        echo -e "\n// FILE: $file\n" >> "${OUTPUT_FILE}"
        cat "$file" >> "${OUTPUT_FILE}"
    fi
}

echo "Adding compiled source files..."

# Core translation units only
add_file "main.cpp"
add_file "window.h" 
add_file "window.cpp"
add_file "scene.h"
add_file "scene.cpp" 
add_file "view.h"
add_file "view.cpp"
add_file "node.h"
add_file "node.cpp"
add_file "socket.h" 
add_file "socket.cpp"
add_file "edge.h"
add_file "edge.cpp"
add_file "graph_factory.h"
add_file "graph_factory.cpp"
add_file "node_templates.h"
add_file "node_templates.cpp" 
add_file "graph_observer.h"
add_file "graph_observer.cpp"
add_file "xml_autosave_observer.h"
add_file "xml_autosave_observer.cpp"
add_file "node_palette_widget.h"
add_file "node_palette_widget.cpp"
add_file "ghost_edge.h"
add_file "ghost_edge.cpp"

# Build config
add_file "CMakelists.txt"

# Resources
add_file "icons.qrc"

echo -e "\n// END OF COMPILED SOURCE" >> "${OUTPUT_FILE}"

file_count=$(grep -c "^// FILE:" "${OUTPUT_FILE}")
echo ""
echo "Short compendium complete!"
echo "Files: ${file_count}"
echo "Output: ${OUTPUT_FILE}"