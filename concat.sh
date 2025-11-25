#!/bin/bash

# Concatenate all source files in translation unit order
# This script creates a complete code review package matching CMakeLists.txt
# Includes: .h, .cpp, .qrc, and build configuration files only

OUTPUT="concatenated_code.txt"

# Remove previous version if it exists
rm -f "$OUTPUT"

echo "Creating concatenated source file: $OUTPUT"
echo "Following translation unit order from CMakeLists.txt"
echo ""

# Helper function to add a file with header
add_file() {
    local file="$1"
    if [ -f "$file" ]; then
        echo "/// ===== FILE: $file =====" >> "$OUTPUT"
        cat "$file" >> "$OUTPUT"
        echo -e "\n\n" >> "$OUTPUT"
        echo "  ✓ $file"
    else
        echo "  ✗ $file (not found)"
    fi
}

# Add section header
add_section() {
    echo "" >> "$OUTPUT"
    echo "/// ============================================================" >> "$OUTPUT"
    echo "/// $1" >> "$OUTPUT"
    echo "/// ============================================================" >> "$OUTPUT"
    echo "" >> "$OUTPUT"
    echo ""
    echo "=== $1 ==="
}

# ─────────────────────────────────────
# Build Configuration
# ─────────────────────────────────────
add_section "BUILD CONFIGURATION"
add_file "CMakeLists.txt"
add_file "build.bat"
add_file "build.sh"

# ─────────────────────────────────────
# Main Application Entry Point
# ─────────────────────────────────────
add_section "MAIN APPLICATION"
add_file "main.cpp"

# ─────────────────────────────────────
# Self-Serializing Graph Architecture
# ─────────────────────────────────────
add_section "SELF-SERIALIZING ARCHITECTURE"
add_file "node.h"
add_file "node.cpp"
add_file "socket.h"
add_file "socket.cpp"
add_file "edge.h"
add_file "edge.cpp"
add_file "ghost_edge.h"
add_file "ghost_edge.cpp"

# ─────────────────────────────────────
# XML-First Factory Pattern
# ─────────────────────────────────────
add_section "XML-FIRST FACTORY"
add_file "graph_factory.h"
add_file "graph_factory.cpp"

# ─────────────────────────────────────
# Graph Facade (Public API + JavaScript Integration)
# ─────────────────────────────────────
add_section "GRAPH FACADE (PUBLIC API)"
add_file "graph.h"
add_file "graph.cpp"

# ─────────────────────────────────────
# Template-Driven Node Types
# ─────────────────────────────────────
add_section "TEMPLATE-DRIVEN NODE TYPES"
add_file "node_templates.h"
add_file "node_templates.cpp"

# ─────────────────────────────────────
# UI Layer
# ─────────────────────────────────────
add_section "UI LAYER"
add_file "window.h"
add_file "window.cpp"
add_file "view.h"
add_file "view.cpp"
add_file "scene.h"
add_file "scene.cpp"

# ─────────────────────────────────────
# Node Palette Widget
# ─────────────────────────────────────
add_section "NODE PALETTE"
add_file "node_palette_widget.h"
add_file "node_palette_widget.cpp"

# ─────────────────────────────────────
# JavaScript Integration (REMOVED - now part of Graph facade)
# ─────────────────────────────────────
# Old files (script_api_stub.h, script_host.h, etc.) no longer exist
# JavaScript integration is now built into graph.h/graph.cpp via QJSEngine

# ─────────────────────────────────────
# Observer Pattern
# ─────────────────────────────────────
add_section "OBSERVER PATTERN"
add_file "graph_observer.h"
add_file "graph_observer.cpp"
add_file "xml_autosave_observer.h"
add_file "xml_autosave_observer.cpp"

# ─────────────────────────────────────
# Resources
# ─────────────────────────────────────
add_section "RESOURCES"
add_file "icons.qrc"

# ─────────────────────────────────────
# Scripts & Automation Helpers
# ─────────────────────────────────────
add_section "SCRIPTS & AUTOMATION"
add_file "scripts/generate_graph.py"
add_file "scripts/coverage_report.sh"
add_file "scripts/coverage_scenario.js"

echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "✓ Done: All translation unit sources written to $OUTPUT"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

# Print file statistics
if [ -f "$OUTPUT" ]; then
    lines=$(wc -l < "$OUTPUT")
    size=$(du -h "$OUTPUT" | cut -f1)
    echo "  Lines: $lines"
    echo "  Size:  $size"
fi
