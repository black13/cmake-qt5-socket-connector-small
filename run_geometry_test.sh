#!/bin/bash
# Run geometry discipline test script

cd "$(dirname "$0")/build_linux" || exit 1

echo "Running geometry discipline test..."
echo "This will open the GUI - check the graph visually, then close the window."
echo ""

DISPLAY=:0 ./NodeGraph --script ../scripts/test_geometry_discipline.js

echo ""
echo "Check logs/JavaScript_*.log for test output"
