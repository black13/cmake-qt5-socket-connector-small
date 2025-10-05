#!/bin/bash
# Shell script to run the complete graph example from WSL/Linux
# This creates a graph with all 5 node types and 0 empty sockets

set -e

echo "================================================"
echo "Running Complete Graph Example (WSL/Linux)"
echo "================================================"
echo ""

# Check if executable exists
if [ ! -f "build_Debug/Debug/NodeGraph.exe" ]; then
    echo "ERROR: NodeGraph.exe not found!"
    echo "Please build the project first"
    exit 1
fi

# Check if script exists
if [ ! -f "scripts/example_complete_graph.js" ]; then
    echo "ERROR: Script not found!"
    echo "Expected: scripts/example_complete_graph.js"
    exit 1
fi

echo "Launching NodeGraph with complete graph script..."
echo ""

# Run the Windows executable from WSL (should work since we're in /mnt/c/)
# Run in background and capture output
./build_Debug/Debug/NodeGraph.exe --script scripts/example_complete_graph.js 2>&1 &
PID=$!

# Wait a few seconds for script to execute
echo "Waiting for script execution (PID: $PID)..."
sleep 5

# Check if process is still running
if ps -p $PID > /dev/null; then
    echo "Application is still running. Waiting a bit more..."
    sleep 5
fi

echo ""
echo "================================================"
echo "Checking Output Files"
echo "================================================"
echo ""

# Check for graph XML file
if [ -f "complete_graph.xml" ]; then
    echo "✓ Graph saved: complete_graph.xml"
    echo ""
    echo "Preview of saved graph:"
    echo "----------------------------------------"
    head -n 20 complete_graph.xml
    echo "..."
    echo "----------------------------------------"
    echo ""

    # Count nodes and edges
    NODE_COUNT=$(grep -c '<node' complete_graph.xml || true)
    EDGE_COUNT=$(grep -c '<edge' complete_graph.xml || true)
    echo "Graph statistics from XML:"
    echo "  Nodes: $NODE_COUNT"
    echo "  Edges: $EDGE_COUNT"
else
    echo "⚠ complete_graph.xml not found"
fi

echo ""

# Check for log files
if [ -d "logs" ]; then
    LATEST_JS_LOG=$(ls -t logs/JavaScript_*.log 2>/dev/null | head -1)

    if [ -n "$LATEST_JS_LOG" ]; then
        echo "✓ JavaScript log: $(basename $LATEST_JS_LOG)"
        echo ""
        echo "Script execution log (last 40 lines):"
        echo "----------------------------------------"
        tail -n 40 "$LATEST_JS_LOG"
        echo "----------------------------------------"
    else
        echo "⚠ No JavaScript log found"
    fi
else
    echo "⚠ logs/ directory not found"
fi

echo ""
echo "================================================"
echo "Complete!"
echo "================================================"
echo ""
echo "To view the graph in the GUI:"
echo "  ./build_Debug/Debug/NodeGraph.exe --load complete_graph.xml"
echo ""
echo "Or on Windows:"
echo "  .\\build_Debug\\Debug\\NodeGraph.exe --load complete_graph.xml"
