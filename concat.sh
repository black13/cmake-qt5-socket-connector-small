#!/bin/bash

# Output file name
OUTPUT_FILE="concatenated_code.txt"

# Initialize the output file
echo "// === Concatenated Top-Level C++ Code Files ===" > "$OUTPUT_FILE"
echo "// Created on $(date)" >> "$OUTPUT_FILE"
echo >> "$OUTPUT_FILE"

# Loop over .h and .cpp files in the current directory (non-recursive)
for file in *.h *.cpp; do
    # Skip if no matching files
    [ -e "$file" ] || continue

    # Get last modified date
    MOD_DATE=$(stat -c %y "$file" | cut -d'.' -f1)

    # Write C++ comment header for this file
    echo "// ====================================" >> "$OUTPUT_FILE"
    echo "// FILE: $file" >> "$OUTPUT_FILE"
    echo "// LAST MODIFIED: $MOD_DATE" >> "$OUTPUT_FILE"
    echo "// ====================================" >> "$OUTPUT_FILE"
    echo >> "$OUTPUT_FILE"

    # Append file content
    cat "$file" >> "$OUTPUT_FILE"
    echo >> "$OUTPUT_FILE"
    echo >> "$OUTPUT_FILE"
done

echo "? Done. Output written to: $OUTPUT_FILE"
