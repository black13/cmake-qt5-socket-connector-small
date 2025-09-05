#!/bin/bash

# Output file name
OUTPUT="concatenated_code.txt"

# Remove previous version if it exists
rm -f "$OUTPUT"

# Define which extensions to include
EXTENSIONS=("*.h" "*.cpp" "*.cmake" "*.py")

# Concatenate each matching top-level file
for ext in "${EXTENSIONS[@]}"; do
    for file in $ext; do
        if [ -f "$file" ]; then
            echo "/// ===== FILE: $file =====" >> "$OUTPUT"
            cat "$file" >> "$OUTPUT"
            echo -e "\n\n" >> "$OUTPUT"
        fi
    done
done

echo "Done: All top-level source files written to $OUTPUT"
