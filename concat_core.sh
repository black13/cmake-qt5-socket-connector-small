#\!/bin/bash
# Simple concatenation of core translation unit files

OUTPUT_FILE="core_code_concat.txt"

echo "=== Core NodeGraph Source Files ===" > $OUTPUT_FILE
echo "Generated: $(date)" >> $OUTPUT_FILE
echo "" >> $OUTPUT_FILE

# Core source files only
for file in main.cpp window.cpp scene.cpp view.cpp node.cpp socket.cpp edge.cpp graph_factory.cpp node_registry.cpp javascript_engine.cpp graph_controller.cpp; do
    if [[ -f "$file" ]]; then
        echo "=== $file ===" >> $OUTPUT_FILE
        cat "$file" >> $OUTPUT_FILE
        echo "" >> $OUTPUT_FILE
        echo "" >> $OUTPUT_FILE
    fi
done

# Core headers
for file in window.h scene.h view.h node.h socket.h edge.h graph_factory.h node_registry.h javascript_engine.h graph_controller.h; do
    if [[ -f "$file" ]]; then
        echo "=== $file ===" >> $OUTPUT_FILE
        cat "$file" >> $OUTPUT_FILE
        echo "" >> $OUTPUT_FILE
        echo "" >> $OUTPUT_FILE
    fi
done

echo "Core files concatenated to: $OUTPUT_FILE"
wc -l $OUTPUT_FILE
EOF < /dev/null
