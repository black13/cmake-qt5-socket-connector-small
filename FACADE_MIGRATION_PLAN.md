# Graph Facade Migration Plan
**Date:** October 23, 2025
**Strategy:** Incremental migration via feature branches

---

## Branch Strategy

Each category gets its own feature branch for isolated testing and review.

### Branch 1: `feature/facade-graph-loading`
**Purpose:** Migrate file loading operations
**Changes:** 2 locations
**Risk:** Low
**Base:** main

**Changes:**
1. `loadGraphFromFile()` - Line 307
   - FROM: `m_factory->loadFromXmlFile(filename)`
   - TO: `m_graph->loadFromFile(filename)`

2. `onClearAndLoadFromFile()` - Line 1564
   - FROM: `m_scene->clearGraph(); m_factory->loadFromXmlFile(path);`
   - TO: `m_graph->loadFromFile(path);`

**Test Plan:**
- Load existing graph files
- Verify autosave works
- Check observer notifications

---

### Branch 2: `feature/facade-graph-clearing`
**Purpose:** Migrate graph clearing operations
**Changes:** ~12 locations
**Risk:** Low
**Base:** main (or feature/facade-graph-loading if sequential)

**Changes:**
1. `loadGraphFromFile()` - Line 305
2. `clearGraph()` - Line 930
3. `clearGraphAndQuit()` - Lines 1076, 1164
4. `onTestInvalidEdges()` - Line 1292
5. `onTestBulkCreateDelete()` - Line 1341
6. `onTestRandomGraphGeneration()` - Line 1484
7. `onTestFileLoadSave()` - Line 1551
8. `onClearAndLoadFromFile()` - Line 1563 (remove, handled by loadFromFile)
9. `onExportToPngTest()` - Line 1586
10. `onExportToPngDemoPattern()` - Line 1591
11. `onExportToPngLinearChain()` - Line 1607

**Pattern:**
```cpp
// FROM:
m_scene->clearGraph();

// TO:
m_graph->clearGraph();
```

**Test Plan:**
- Clear graph via menu
- Load new graph (verifies clear + load)
- Run all test functions
- Verify no memory leaks

---

### Branch 3: `feature/facade-query-operations`
**Purpose:** Migrate query operations (getNodes, getEdges, stats)
**Changes:** ~15 locations
**Risk:** Low (read-only operations)
**Base:** main

**Changes:**

**3.1 Status Bar Updates** - Lines 268-269, 844-845
```cpp
// FROM:
int nodeCount = m_scene->getNodes().size();
int edgeCount = m_scene->getEdges().size();

// TO:
QVariantMap stats = m_graph->getGraphStats();
int nodeCount = stats["nodeCount"].toInt();
int edgeCount = stats["edgeCount"].toInt();
```

**3.2 Info Panel** - Lines 248-254
```cpp
// FROM:
for (Node* node : m_scene->getNodes().values()) {
    QString type = node->getNodeType();
    // ...
}

// TO:
QVariantList nodeIds = m_graph->getAllNodes();
for (const QVariant& nodeIdVar : nodeIds) {
    QVariantMap nodeData = m_graph->getNodeData(nodeIdVar.toString());
    QString type = nodeData["type"].toString();
    // ...
}
```

**3.3 Debug Logging** - Multiple locations
- Lines 1072-1073, 1080, 1084, 1089
- Lines 1122, 1160, 1168, 1172, 1177
- Lines 1250-1251, 1336-1338, 1344-1346
- Lines 1378-1379, 1529-1530

All use same pattern as 3.1

**Test Plan:**
- Verify status bar shows correct counts
- Check info panel displays node/edge data
- Run all test functions
- Verify debug logging output

---

### Branch 4: `feature/facade-edge-creation`
**Purpose:** Migrate edge creation to Graph facade
**Changes:** ~8 locations
**Risk:** Medium (API change from pointers to UUIDs)
**Base:** main

**Changes:**

**4.1 Test Functions - Edge Creation**

Lines: 1210, 1222, 1234, 1246, 1303, 1333, 1368, 1515, 1627

```cpp
// FROM:
Edge* edge1 = m_factory->createEdge(sourceNode, 0, transformNode, 0);
if (!edge1) { /* error */ }

// TO:
QString edgeId = m_graph->connectNodes(
    sourceNode->getId().toString(QUuid::WithBraces), 0,
    transformNode->getId().toString(QUuid::WithBraces), 0
);
if (edgeId.isEmpty()) { /* error */ }
```

**4.2 Batch Operations** - Line 1333
```cpp
// Use batch mode for bulk edge creation
m_graph->beginBatch();
for (int i = 0; i < 10; ++i) {
    QString edgeId = m_graph->connectNodes(aId, 0, bId, 0);
}
m_graph->endBatch();
```

**Test Plan:**
- Run all edge creation tests
- Verify edges render correctly
- Test batch mode performance
- Check edge deletion still works
- Verify autosave triggers

---

## Operations NOT Migrated (Intentionally)

### Keep Direct Scene Access:
```cpp
// Qt-Specific Operations (KEEP AS-IS)
m_scene->selectedItems()          // Returns QGraphicsItem*
m_scene->itemsBoundingRect()      // Qt graphics
m_scene->render(&painter, ...)    // Qt rendering
m_scene->setSnapToGrid(bool)      // Qt scene property
m_scene->items().size()           // Qt item count (different from node count)

// Initialization/Setup (KEEP AS-IS)
m_scene->setGraphFactory()        // One-time setup
m_scene->attach/detach()          // Observer setup
m_scene->prepareForShutdown()     // Cleanup

// Auto-layout (NOT IN FACADE YET - KEEP AS-IS)
m_scene->autoLayoutAnneal()
m_scene->autoLayoutForceDirected()
m_scene->debugForceLayout3Nodes()

// View Operations (KEEP AS-IS)
m_view->fitInView(...)
```

### Keep Direct Factory Access (Test Functions):
```cpp
// Test functions explicitly testing Factory internals
// Lines: 1105, 1118, 1185-1188, 1294-1295, 1325-1326, 1365-1366, 1498, 1620
m_factory->createNode()  // Tests can keep direct access for now
```

---

## Merge Strategy

**Recommended: Sequential Merge**
```
main
  ├─→ feature/facade-graph-loading → merge to main
  ├─→ feature/facade-graph-clearing → merge to main
  ├─→ feature/facade-query-operations → merge to main
  └─→ feature/facade-edge-creation → merge to main
```
- Each branch builds on previous work
- Can test incrementally
- Easy to isolate issues

---

## Testing Checklist (Per Branch)

**Build Test:**
- [ ] `build.bat debug` - Clean build
- [ ] No new warnings
- [ ] All files compile

**Basic Operations:**
- [ ] Create nodes via palette
- [ ] Create nodes via menu (Ctrl+1, Ctrl+2, Ctrl+3)
- [ ] Create edges (right-click drag)
- [ ] Move nodes
- [ ] Delete nodes/edges
- [ ] Save graph (Ctrl+S)
- [ ] Load graph (Ctrl+L)
- [ ] Clear graph (Ctrl+N)

**Menu Tests:**
- [ ] Run all test menu items
- [ ] No crashes
- [ ] Check logs for errors

**Regression Check:**
- [ ] Compare logs before/after
- [ ] Same functionality
- [ ] No new QColor warnings
- [ ] Autosave working

---

## Migration Statistics

| Branch | Changes | Risk | Estimated Time |
|--------|---------|------|----------------|
| graph-loading | 2 | Low | 10 min |
| graph-clearing | 12 | Low | 15 min |
| query-operations | 15 | Low | 20 min |
| edge-creation | 8 | Medium | 20 min |
| **TOTAL** | **37** | **Low-Med** | **~65 min** |

---

## Expected Benefits

1. **Unified API:** All graph operations through single facade
2. **JavaScript Ready:** All operations callable from JS
3. **Type Safety:** UUID strings instead of raw pointers
4. **Validation:** Can add validation in Graph facade
5. **Logging:** Centralized logging for debugging
6. **Consistency:** Same API for C++, JavaScript, and future clients

---

## Current Status

- [x] Plan created
- [ ] Branch 1: feature/facade-graph-loading
- [ ] Branch 2: feature/facade-graph-clearing
- [ ] Branch 3: feature/facade-query-operations
- [ ] Branch 4: feature/facade-edge-creation
- [ ] All branches merged to main
- [ ] Documentation updated

---

**Next Step:** Create and work on Branch 1: feature/facade-graph-loading
