#!/bin/bash

# Output file name
OUTPUT="concatenated_code.txt"

# Remove previous version if it exists
rm -f "$OUTPUT"

# Files that are actually in the build (from CMakeLists.txt)
BUILD_FILES=(
  # Core architecture
  node.h node.cpp
  socket.h socket.cpp
  edge.h edge.cpp
  ghost_edge.h ghost_edge.cpp

  # Graph architecture
  qgraph.h qgraph.cpp
  graph_factory.h graph_factory.cpp

  # UI components
  window.h window.cpp
  view.h view.cpp
  scene.h scene.cpp
  node_palette_widget.h node_palette_widget.cpp

  # JavaScript integration
  javascript_engine.h javascript_engine.cpp

  # Observer pattern
  graph_observer.h graph_observer.cpp
  xml_autosave_observer.h xml_autosave_observer.cpp

  # Main entry point
  main.cpp
)

# Concatenate each file in the order defined
for file in "${BUILD_FILES[@]}"; do
    if [ -f "$file" ]; then
        echo "/// ===== FILE: $file =====" >> "$OUTPUT"
        cat "$file" >> "$OUTPUT"
        echo -e "\n\n" >> "$OUTPUT"
    else
        echo "Warning: File not found: $file"
    fi
done

echo "Done: All translation unit source files written to $OUTPUT"
