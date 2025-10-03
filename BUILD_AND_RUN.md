# Build and Run Complete Graph Example

## Step 1: Build the Application (Windows)

### Using Visual Studio
```powershell
cd C:\temp\cmake-qt5-socket-connector-small\build_Debug

# Open in Visual Studio
start NodeGraph.sln

# Then in Visual Studio:
# - Right-click "NodeGraph" project → Set as Startup Project
# - Build → Build Solution (F7)
# - Or Build → Build NodeGraph
```

### Using MSBuild (Command Line)
```powershell
cd C:\temp\cmake-qt5-socket-connector-small\build_Debug

# Build the solution
msbuild NodeGraph.sln /p:Configuration=Debug

# Or build just the NodeGraph target
msbuild NodeGraph.vcxproj /p:Configuration=Debug
```

### Expected Output
After building, you should see:
- `Debug\NodeGraph.exe` (the main executable)
- `Debug\NodeGraphCore.lib` (the core library)
- Qt DLLs copied to Debug folder

## Step 2: Run the Complete Graph Script

### From PowerShell
```powershell
cd C:\temp\cmake-qt5-socket-connector-small

# Run with script
.\build_Debug\Debug\NodeGraph.exe --script scripts/example_complete_graph.js
```

### From WSL (if cross-platform)
```bash
cd /mnt/c/temp/cmake-qt5-socket-connector-small

# Run with script
./build_Debug/Debug/NodeGraph.exe --script scripts/example_complete_graph.js
```

## Step 3: Check the Output

### 1. Console Output
You should see the script execution in the application console window.

### 2. Saved Graph File
```powershell
# View the saved graph
cat complete_graph.xml

# Or open in notepad
notepad complete_graph.xml
```

### 3. Log Files
```powershell
# Check logs directory
ls logs\

# View latest log
cat (ls logs\NodeGraph_*.log | sort -Descending | select -First 1)

# View JavaScript log
cat (ls logs\JavaScript_*.log | sort -Descending | select -First 1)
```

### 4. Load and View in GUI
```powershell
# Open the saved graph in the GUI
.\build_Debug\Debug\NodeGraph.exe --load complete_graph.xml
```

## Alternative: Run from Visual Studio

### Method 1: Set Command Line Arguments
1. Right-click "NodeGraph" project → Properties
2. Configuration Properties → Debugging
3. Command Arguments: `--script scripts/example_complete_graph.js`
4. Working Directory: `$(ProjectDir)..\..`
5. Click OK
6. Press F5 to run

### Method 2: Run Directly
1. Build the solution (F7)
2. Open PowerShell in repository root
3. Run: `.\build_Debug\Debug\NodeGraph.exe -s scripts/example_complete_graph.js`

## Expected Output

When the script runs, you should see:

```
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
Example: Complete Graph - All Types, No Empty Sockets
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
Graph cleared

[Step 1] Creating nodes...
✅ SOURCE at (100, 250)
   Sockets: 0 inputs, 1 output
   - Socket 0: OUTPUT
✅ SPLIT at (300, 250)
   Sockets: 1 input, 2 outputs
   - Socket 0: INPUT
   - Socket 1: OUTPUT 1
   - Socket 2: OUTPUT 2
✅ TRANSFORM 1 at (500, 150)
   Sockets: 1 input, 1 output
   - Socket 0: INPUT
   - Socket 1: OUTPUT
✅ TRANSFORM 2 at (500, 350)
   Sockets: 1 input, 1 output
   - Socket 0: INPUT
   - Socket 1: OUTPUT
✅ MERGE at (700, 250)
   Sockets: 2 inputs, 1 output
   - Socket 0: INPUT 1
   - Socket 1: INPUT 2
   - Socket 2: OUTPUT
✅ SINK at (900, 250)
   Sockets: 1 input, 0 outputs
   - Socket 0: INPUT

[Step 2] Connecting all sockets...
✅ Edge 1: SOURCE[0] → SPLIT[0]
   SOURCE output connected ✓
   SPLIT input connected ✓
✅ Edge 2: SPLIT[1] → TRANSFORM1[0]
   SPLIT output 1 connected ✓
   TRANSFORM1 input connected ✓
✅ Edge 3: SPLIT[2] → TRANSFORM2[0]
   SPLIT output 2 connected ✓
   TRANSFORM2 input connected ✓
✅ Edge 4: TRANSFORM1[1] → MERGE[0]
   TRANSFORM1 output connected ✓
   MERGE input 1 connected ✓
✅ Edge 5: TRANSFORM2[1] → MERGE[1]
   TRANSFORM2 output connected ✓
   MERGE input 2 connected ✓
✅ Edge 6: MERGE[2] → SINK[0]
   MERGE output connected ✓
   SINK input connected ✓

[Step 3] Verifying graph completeness...
Graph statistics:
  Nodes: 6 (expected: 6)
  Edges: 6 (expected: 6)

Node type distribution:
  SOURCE: 1 (expected: 1)
  SINK: 1 (expected: 1)
  TRANSFORM: 2 (expected: 2)
  SPLIT: 1 (expected: 1)
  MERGE: 1 (expected: 1)

Socket analysis:
  Total sockets: 12
  Connected sockets: 12
  Empty sockets: 0

Edge details:
  Edge 1: from node[0] → to node[0]
  Edge 2: from node[1] → to node[0]
  Edge 3: from node[2] → to node[0]
  Edge 4: from node[1] → to node[0]
  Edge 5: from node[1] → to node[1]
  Edge 6: from node[2] → to node[0]

==================================================
VALIDATION RESULTS:
==================================================
✅ All 5 node types present
✅ All sockets connected (no empty sockets)
✅ Correct node and edge counts

🎉 SUCCESS: Complete graph with all types and no empty sockets!
==================================================

✅ Graph saved to complete_graph.xml

[Visual Representation]

    SOURCE (0,1)
        │
        │ output 0
        ▼
    SPLIT (1,2)
        │
   ┌────┴────┐
   │         │
out 1      out 2
   │         │
   ▼         ▼
TRANS1    TRANS2
(1,1)     (1,1)
   │         │
out 1      out 1
   │         │
   └────┬────┘
        │
     MERGE (2,1)
        │
        │ output 2
        ▼
     SINK (1,0)

Legend: TYPE (inputs, outputs)

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
Complete Graph Example Finished!
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
```

## Viewing the Generated XML

The `complete_graph.xml` file should look like:

```xml
<?xml version="1.0" encoding="UTF-8"?>
<graph version="1.0">
  <node type="SOURCE" id="{...}" x="100" y="250" inputs="0" outputs="1"/>
  <node type="SPLIT" id="{...}" x="300" y="250" inputs="1" outputs="2"/>
  <node type="TRANSFORM" id="{...}" x="500" y="150" inputs="1" outputs="1"/>
  <node type="TRANSFORM" id="{...}" x="500" y="350" inputs="1" outputs="1"/>
  <node type="MERGE" id="{...}" x="700" y="250" inputs="2" outputs="1"/>
  <node type="SINK" id="{...}" x="900" y="250" inputs="1" outputs="0"/>

  <edge id="{...}" from="{SOURCE-id}" to="{SPLIT-id}" fromSocket="0" toSocket="0"/>
  <edge id="{...}" from="{SPLIT-id}" to="{TRANS1-id}" fromSocket="1" toSocket="0"/>
  <edge id="{...}" from="{SPLIT-id}" to="{TRANS2-id}" fromSocket="2" toSocket="0"/>
  <edge id="{...}" from="{TRANS1-id}" to="{MERGE-id}" fromSocket="1" toSocket="0"/>
  <edge id="{...}" from="{TRANS2-id}" to="{MERGE-id}" fromSocket="1" toSocket="1"/>
  <edge id="{...}" from="{MERGE-id}" to="{SINK-id}" fromSocket="2" toSocket="0"/>
</graph>
```

## Troubleshooting

### Build Errors
**Qt not found:**
- Check CMakeLists.txt has correct Qt path (line 106-109)
- Ensure Qt 5.15.16 is installed at the specified location

**libxml2 errors:**
- CMake should automatically download libxml2 via FetchContent
- Check internet connection during first build

### Runtime Errors
**Script not found:**
- Make sure you're running from the repository root
- Check that `scripts/example_complete_graph.js` exists

**No console output:**
- The application may be running in GUI mode
- Check logs in `logs/` directory
- Try adding more console.log statements to the script

**Graph not saved:**
- Check write permissions in current directory
- Look for `complete_graph.xml` in the working directory
- Check the application's debug output for save errors

## Next Steps

After successfully running the complete graph example:

1. **Try other examples:**
   ```powershell
   .\build_Debug\Debug\NodeGraph.exe -s scripts/example_simple_chain.js
   .\build_Debug\Debug\NodeGraph.exe -s scripts/example_diamond_pattern.js
   .\build_Debug\Debug\NodeGraph.exe -s scripts/example_stress_test.js
   ```

2. **Create your own script:**
   - Copy `example_complete_graph.js`
   - Modify the node positions and connections
   - Run it with `--script your_script.js`

3. **Visualize the graphs:**
   - Load saved XML files: `--load your_graph.xml`
   - Use the GUI to see the visual representation
   - Drag nodes around and save changes
