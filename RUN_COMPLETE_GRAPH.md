# Running the Complete Graph Example

## Quick Start

### Windows (PowerShell)
```powershell
cd C:\temp\cmake-qt5-socket-connector-small

# Run the complete graph script
.\build_Debug\NodeGraph.exe --script scripts/example_complete_graph.js

# Or use short form
.\build_Debug\NodeGraph.exe -s scripts/example_complete_graph.js
```

### Linux/WSL
```bash
cd /mnt/c/temp/cmake-qt5-socket-connector-small

# Run the complete graph script
./build_Debug/NodeGraph --script scripts/example_complete_graph.js

# Or use short form
./build_Debug/NodeGraph -s scripts/example_complete_graph.js
```

## What Happens

1. **Application starts** with logging enabled
2. **Script executes automatically** on window show
3. **Complete graph is created**:
   - 1 SOURCE node
   - 1 SINK node
   - 2 TRANSFORM nodes
   - 1 SPLIT node
   - 1 MERGE node
   - 6 edges connecting all 12 sockets
4. **Graph is saved** to `complete_graph.xml`
5. **Logs are written** to `logs/` directory

## Output Files

### Graph File
- **Location**: `./complete_graph.xml`
- **Content**: Complete graph with all 5 node types
- **Format**: libxml2 XML format

### Log Files
- **Main log**: `logs/NodeGraph_YYYY-MM-DD_HH-MM-SS.log`
- **JavaScript log**: `logs/JavaScript_YYYY-MM-DD_HH-MM-SS.log`

### Console Output
The script will print:
```
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
Example: Complete Graph - All Types, No Empty Sockets
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

[Step 1] Creating nodes...
✅ SOURCE at (100, 250)
   Sockets: 0 inputs, 1 output
   - Socket 0: OUTPUT
✅ SPLIT at (300, 250)
   Sockets: 1 input, 2 outputs
   - Socket 0: INPUT
   - Socket 1: OUTPUT 1
   - Socket 2: OUTPUT 2
...

[Step 2] Connecting all sockets...
✅ Edge 1: SOURCE[0] → SPLIT[0]
✅ Edge 2: SPLIT[1] → TRANSFORM1[0]
...

[Step 3] Verifying graph completeness...
Graph statistics:
  Nodes: 6 (expected: 6)
  Edges: 6 (expected: 6)

Socket analysis:
  Total sockets: 12
  Connected sockets: 12
  Empty sockets: 0

VALIDATION RESULTS:
✅ All 5 node types present
✅ All sockets connected (no empty sockets)
✅ Correct node and edge counts

🎉 SUCCESS: Complete graph with all types and no empty sockets!
```

## Viewing the Results

### 1. Check the saved graph
```bash
cat complete_graph.xml
```

### 2. Check the logs
```bash
# Main application log
cat logs/NodeGraph_*.log | tail -100

# JavaScript execution log
cat logs/JavaScript_*.log
```

### 3. Load in GUI
```bash
# Load the saved graph in the GUI
./build_Debug/NodeGraph --load complete_graph.xml

# Or
./build_Debug/NodeGraph -l complete_graph.xml
```

## Other Example Scripts

Run any of the example scripts the same way:

```bash
# Simple chain
./build_Debug/NodeGraph -s scripts/example_simple_chain.js

# Fan-out pattern
./build_Debug/NodeGraph -s scripts/example_fan_out.js

# Fan-in pattern
./build_Debug/NodeGraph -s scripts/example_fan_in.js

# Diamond pattern
./build_Debug/NodeGraph -s scripts/example_diamond_pattern.js

# Grid pattern (3x3)
./build_Debug/NodeGraph -s scripts/example_grid_pattern.js

# Stress test (50+ nodes)
./build_Debug/NodeGraph -s scripts/example_stress_test.js
```

## Headless Mode (Future)

Currently, the application requires a GUI window. For truly headless execution, we would need to add a `--headless` or `--no-gui` flag that:
1. Skips window.show()
2. Runs the script
3. Exits after script completion

This could be added in a future update.

## Troubleshooting

### Script not found
```
Error: Cannot find script: scripts/example_complete_graph.js
```
**Solution**: Make sure you're running from the repository root where the `scripts/` directory exists.

### Build not found
```
bash: ./build_Debug/NodeGraph: No such file or directory
```
**Solution**: Build the project first or adjust the path to your build directory.

### No output in logs
**Solution**: Check that the `logs/` directory was created. The application creates it automatically, but you can create it manually:
```bash
mkdir -p logs
```

### Graph not saved
**Solution**: Check the console output. If the script ran but didn't save, check file permissions in the current directory.

## Advanced Usage

### Run script and load existing graph
```bash
# Load a graph, then run a script on it
./build_Debug/NodeGraph -l my_graph.xml -s scripts/analyze.js
```

### Multiple tests
```bash
# Run multiple scripts sequentially (via shell script)
for script in scripts/example_*.js; do
    echo "Running $script..."
    ./build_Debug/NodeGraph -s "$script"
    sleep 2  # Wait between tests
done
```
