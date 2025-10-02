# Test Plan: XML Graph Load and Enumeration

## Purpose

Temporary test to validate JavaScript system health by loading an XML graph file and enumerating all nodes and edges.

## Objectives

1. **Verify JavaScript Integration**: Confirm QGraph JavaScript API is working correctly
2. **Validate XML Loading**: Test that graph can be loaded from XML file
3. **Enumerate Graph Elements**: List all nodes and edges with their properties
4. **Health Check**: Verify graph structure integrity after load

## Test Implementation

### Test Script: `scripts/test_xml_load_enumeration.js`

**Steps:**
1. Load a known XML graph file (e.g., `test_graph.xml`)
2. Get graph statistics (node count, edge count)
3. Enumerate all nodes:
   - Node ID (UUID)
   - Node type
   - Position (x, y)
   - Socket counts (inputs, outputs)
4. Enumerate all edges:
   - Edge ID (UUID)
   - Source node + socket index
   - Target node + socket index
5. Validate integrity:
   - All edges reference valid nodes
   - Socket indices are in valid range
   - No orphaned elements

### Sample XML Graph: `test_graph.xml`

**Graph Structure:**
```
SOURCE -> TRANSFORM -> SINK
  |
  +----> LOGGER
```

**Contents:**
- 4 nodes (SOURCE, TRANSFORM, SINK, LOGGER)
- 3 edges (SOURCE->TRANSFORM, TRANSFORM->SINK, SOURCE->LOGGER)

## Expected Output

```
=== XML Load and Enumeration Test ===
Loading: test_graph.xml

Graph Statistics:
  Nodes: 4
  Edges: 3

Node Enumeration:
  [1/4] Node: {uuid} Type: SOURCE Position: (100, 100) Inputs: 0 Outputs: 1
  [2/4] Node: {uuid} Type: TRANSFORM Position: (250, 100) Inputs: 1 Outputs: 1
  [3/4] Node: {uuid} Type: SINK Position: (400, 100) Inputs: 1 Outputs: 0
  [4/4] Node: {uuid} Type: LOGGER Position: (250, 200) Inputs: 1 Outputs: 0

Edge Enumeration:
  [1/3] Edge: {uuid} From: {source_uuid}[0] To: {transform_uuid}[0]
  [2/3] Edge: {uuid} From: {transform_uuid}[0] To: {sink_uuid}[0]
  [3/3] Edge: {uuid} From: {source_uuid}[0] To: {logger_uuid}[0]

Integrity Check:
  ✓ All edges reference valid nodes
  ✓ All socket indices are valid
  ✓ No orphaned edges

✅ TEST PASSED
```

## Success Criteria

- ✅ XML file loads without errors
- ✅ Node count matches expected
- ✅ Edge count matches expected
- ✅ All nodes enumerated with correct properties
- ✅ All edges enumerated with valid connections
- ✅ No JavaScript exceptions
- ✅ No segfaults or crashes

## Failure Scenarios

- ❌ XML file not found or invalid
- ❌ Node/edge count mismatch
- ❌ JavaScript API methods unavailable
- ❌ Edge references non-existent node
- ❌ Socket index out of range
- ❌ Exception during enumeration

## QGraph API Methods Required

The test validates these JavaScript API methods:
- `Graph.loadXml(filename)` - Load graph from XML
- `Graph.getStats()` - Get node/edge counts
- `Graph.getAllNodes()` - Enumerate all nodes (if available)
- `Graph.getAllEdges()` - Enumerate all edges (if available)
- `Graph.getNodeInfo(uuid)` - Get node properties
- `Graph.getEdgeInfo(uuid)` - Get edge properties

## Branch Strategy

**Branch**: `test/xml-load-enumeration` (from `feat/graph-rearch-01`)

**Reason**: Temporary test branch to validate JavaScript system without polluting main feature branch.

**Lifecycle**:
1. Create test script + sample XML
2. Run test and verify
3. If successful: merge test artifacts back OR delete branch (test is temporary)
4. If failures found: fix issues, re-test

## Timeline

1. **Create plan** - Document test approach (this file)
2. **Create test XML** - Sample graph file
3. **Implement test script** - JavaScript enumeration test
4. **Run test** - Execute and capture output
5. **Validate results** - Verify system health
6. **Decision** - Keep or remove test based on results

## Notes

- This is a **temporary** test for validation purposes
- Primary goal: confirm JavaScript system is working after cast-free refactoring
- Secondary goal: demonstrate graph enumeration capability
- Test can be removed once JavaScript system is proven stable
- May be superseded by formal test suite later
