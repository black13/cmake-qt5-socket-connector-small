# NodeGraph Development Plan
**Updated:** 2025-11-06
**PRIMARY GOAL:** Eliminate qgraphicsitem_cast - architectural rot (2 remaining of 17)
**SECONDARY GOAL:** Full JavaScript integration for programmable nodes/edges

---

## 🎯 NEXT STEP (Session Resume)

**Current Status:**
- Branch: `refactor/ghost-edge-typed` (active for Branch 1.5)
- qgraphicsitem_cast count: **2 remaining** (ghost-edge mouse move/release)
- Working tree consolidation: screenshots (`edgeoverlap.png`, `environement.png`) added to repo; generated/local helpers (`concatenated_code.txt`, `generate_test_files.py`) now ignored via .gitignore
- Recent completions: Branch 1.7 merged (factory typed serialization + VS debugger PATH fix)

**Remaining 2 violations:**
1. `scene.cpp:326` - Ghost edge mouse move (Branch 1.5)
2. `scene.cpp:504` - Ghost edge mouse release (Branch 1.5)

**Immediate Prep:**
- [ ] Run `git status` and confirm only intentional files remain; resolve untracked artifacts
- [ ] Run `git grep -n "qgraphicsitem_cast"` to verify only the two scene hits remain
- [ ] Review current ghost-edge flow (`updateGhostEdge`, `finishGhostEdge`, magnet snapping) before refactoring

**RECOMMENDED NEXT TASK:**
- Continue **Branch 1.5: refactor/ghost-edge-typed** - remove the final casts and stabilize ghost-edge UX
  - Implement `Scene::socketAt(const QPointF&)` using typed node/socket collections (no `itemAt()` or casts)
  - Use the helper in ghost-edge mouse move and mouse release handlers
  - Keep magnet snapping and socket-state resets but ensure they rely only on typed accessors
  - Capture a manual test checklist covering drag, invalid targets, snapping, and cancel flows

**Parallel Prep:** Gather notes/screens for Phase 2.6 (view/navigation + spacing) so we can pivot once Branch 1.5 lands.
---

## Observer & Rendering Next Steps (Nov 2025)

### Current Architecture Snapshot
- `Graph` is the public facade coordinating `Scene` and `GraphFactory` (shared API for C++ and JavaScript).
- `Scene` owns QGraphics items but exposes typed collections (`Node*`, `Edge*`) without QObject inheritance.
- `GraphObserver`/`GraphSubject` deliver plain-C++ notifications; observers must not touch QGraphics items.
- Rendering is moving toward snapshot feeds so the QGraphics view consumes value objects, not live scene pointers.

### Guiding Principles
- Keep Node/Edge/Socket free of QObject; rely on explicit registration and deterministic teardown.
- Observers exchange UUIDs, geometry structs, and payload data—never `QGraphicsItem*`.
- `Graph` remains the sole surface for scripting/UI; `Scene` stays an internal detail.
- Batch operations defer notifications via `GraphSubject::beginBatch()` / `endBatch()`.

### Phase 1 – Event Hub Stabilization
1. Introduce a `GraphEventHub` (or extend `GraphSubject`) that publishes typed event structs:
   - `NodeCreated { QUuid id, QString type, QPointF position }`
   - `NodeMoved { QUuid id, QPointF from, QPointF to }`
   - `EdgeCreated { QUuid id, QUuid fromNode, int fromSocket, QUuid toNode, int toSocket }`
2. Route all `Graph` facade mutations (create/delete/connect/disconnect/move/load/save) through the hub.
3. Ensure batch mode coalesces events before emission and document observer expectations.

### Phase 2 – Snapshot-Based Rendering Bridge
1. Build a `GraphSnapshotBuilder` that walks typed collections and produces immutable render data:
   - `SceneSnapshot { QVector<NodeRenderData>, QVector<EdgeRenderData>, GhostEdgeState }`
2. Create a rendering observer (`RenderBridge`) that listens for hub events and requests snapshots when state changes.
3. Update the QGraphics view ("QGraph") to consume snapshots instead of raw `Scene` pointers.
4. Keep ghost-edge interactions typed: add `Scene::socketAt()` and maintain magnet snapping without `itemAt()` casts.

### Phase 3 – Autosave & Persistence Observers
1. Port `XmlAutosaveObserver` to the event hub—remove any direct `Scene::items()` iteration.
2. Fire autosave once per batch when `GraphSubject::endBatch()` runs.
3. Add failure telemetry (path, error messages) without exposing QGraphics objects to observers.

### Phase 4 – Diagnostics, Logging, Undo Preparation
1. Implement a logging observer that records event structs (human-readable plus optional JSON for tooling).
2. Draft undo/redo command wrappers that store event snapshots rather than graphics references.
3. Document the observer/snapshot interplay in `GRAPH_SPECIFICATION.md` and cross-link from `RULES.md`.

### Phase 5 – JavaScript Validation & Examples
1. Add facade test scripts under `scripts/` (and future `examples/`) covering node create/move/delete, edge connect/disconnect, and autosave verification.
2. Keep scripts asynchronous-safe by querying state through facade methods (`Graph::getNodeData`, `Graph::getAllEdges`, etc.).

### Immediate Follow-Up Items
- Eliminate the final `qgraphicsitem_cast` calls in ghost-edge paths via the typed lookup helper.
- Update existing `GraphObserver` implementations (autosave, future undo) to consume the new event structs.
- Refresh documentation once the event hub and snapshot pipeline are in place.


---

## Session Resume Checklist

**Use this checklist when resuming work after computer restart or context loss:**

1. **Understand Current State:**
   - [ ] Run `git status` to see uncommitted changes
   - [ ] Run `git log --oneline -10` to see recent commits
   - [ ] Check current branch with `git branch`
   - [ ] Review ISSUE.md for known bugs
   - [ ] Review this PLAN.MD Progress Tracking section

2. **Rebuild Project:**
   - [ ] Windows: Open Visual Studio solution, rebuild
   - [ ] WSL/Linux: `cd build_linux && cmake .. && make`
   - [ ] Verify build succeeds before making changes

3. **Validate Current Code:**
   - [ ] Run concatenation: `bash concat.sh` (generates concatenated_code.txt)
   - [ ] Check for dead code with grep: `git grep -n "qgraphicsitem_cast"`
   - [ ] Check test files exist: `ls -la test_*.xml test_*.js`

4. **Memory Leak Testing (WSL/Linux only):**
   - [ ] Build with ASAN: `cmake -DCMAKE_BUILD_TYPE=Debug .. && make`
   - [ ] Run leak test: `LSAN_OPTIONS=suppressions=../asan_suppressions.txt ./NodeGraph test_corrupt_duplicate.xml`
   - [ ] Verify no leaks from graph_factory.cpp

5. **Quick Smoke Test:**
   - [ ] Launch application: `./NodeGraph` (Linux) or run from Visual Studio (Windows)
   - [ ] Test basic operations: create nodes, connect sockets, save/load
   - [ ] Test JavaScript CLI: `./NodeGraph --script test_memory_leak.js`

6. **Identify Next Task:**
   - [ ] Look at Progress Tracking section below
   - [ ] Pick next unchecked [ ] branch
   - [ ] Read the branch description and implementation plan
   - [ ] Create branch: `git checkout -b <branch-name>`

---

## Project Vision

**Architecture First:**
- [ ] ZERO qgraphicsitem_cast violations (17 found - CRITICAL)
- [ ] Typed collections everywhere (Node*, Edge*, Socket*)
- [ ] Objects manage their own lifecycle
- [ ] No type checking loops

**JavaScript Integration:**
- [x] Graph operations callable from JavaScript (Graph facade with QJSEngine)
- [ ] Individual nodes have custom JavaScript behavior
- [ ] Nodes/edges have dynamic payloads (properties, weights, metadata)
- [ ] Users script node logic without recompiling C++
- [ ] Complete application scriptable end-to-end

---

## Completed Work ✅

### Facade Migration (100% Complete)
- [x] Branch 1: Graph loading migrated to facade (commit: 4d55d48)
- [x] Branch 2: Graph clearing migrated to facade (commit: 770ac19)
- [x] Branch 3: Query operations migrated to facade (commit: b5ec77d)
- [x] Branch 4: Edge creation migrated to facade (commit: acfe762)
- [x] All window.cpp operations use Graph facade API
- [x] JavaScript CLI execution via `--script` option (commit: a88c25a)

### Test Code Cleanup
- [x] Removed scene.cpp test code (debugForceLayout3Nodes) (commit: 3d55337)
- [x] Removed ~713 lines of C++ test functions from window.cpp (commit: 462e3df)
- [x] Removed test menus and #if ENABLE_JS blocks
- [x] All testing now via JavaScript + Graph facade
- [x] Deleted obsolete plan files (commit: 0221c2d)
  - FACADE_MIGRATION_PLAN.md (completed)
  - plan_response.md (merged into this plan)
  - ISSUE.md (bugs merged into Phase 1)
  - SESSION_STATE.md (session notes)

**Net cleanup:** -2,711 lines, +1,225 lines

---

## Phase 1: Architecture - Eliminate qgraphicsitem_cast 🔥

**CRITICAL:** 17 violations found - fix them ALL

**Principle:** Objects manage their own lifecycle through typed collections, never through type checking or casting.

### Branch 1.1: Socket Parent Linkage
**Branch:** `refactor/socket-typed-parent`

**Problem:** socket.cpp:42 uses `qgraphicsitem_cast<Node*>(parentItem())`

**Changes:**
- [ ] socket.h: Add `Node* m_parentNode` private member
- [ ] socket.cpp: Initialize in constructor from parent
- [ ] socket.cpp: Remove qgraphicsitem_cast, use m_parentNode directly
- [ ] node.cpp: Pass `this` to Socket constructor

**Eliminates:** 1 cast violation

**Git Workflow:**
```bash
git checkout main
git checkout -b refactor/ghost-edge-typed
# ... make changes ...
git add scene.h scene.cpp
git commit -m "refactor: replace ghost-edge casts with typed lookup

- add Scene::socketAt() helper using typed collections
- route ghost-edge hover/release through typed socket lookup
- keep magnet snapping + state reset paths on typed sockets
- eliminates final qgraphicsitem_cast occurrences"
git push origin refactor/ghost-edge-typed
git checkout main
git merge refactor/ghost-edge-typed --no-ff
git push origin main
git branch -d refactor/ghost-edge-typed
```

---

### Branch 1.2: Node Socket Collections
**Branch:** `refactor/node-typed-collections`

**Problem:** node.cpp:188, 426 iterate childItems() with qgraphicsitem_cast<Socket*>

**Changes:**
- [ ] node.h: Add `QList<Socket*> m_inputSockets` and `QList<Socket*> m_outputSockets`
- [ ] node.h: Add `QList<Socket*> inputSockets()` and `outputSockets()` accessors
- [ ] node.cpp: Store sockets in typed lists when creating them
- [ ] node.cpp: Remove childItems() iteration with casts
- [ ] node.cpp: Use typed lists instead

**Eliminates:** 2 cast violations

**Git Workflow:**
```bash
git checkout main
git checkout -b refactor/ghost-edge-typed
# ... make changes ...
git add scene.h scene.cpp
git commit -m "refactor: replace ghost-edge casts with typed lookup

- add Scene::socketAt() helper using typed collections
- route ghost-edge hover/release through typed socket lookup
- keep magnet snapping + state reset paths on typed sockets
- eliminates final qgraphicsitem_cast occurrences"
git push origin refactor/ghost-edge-typed
git checkout main
git merge refactor/ghost-edge-typed --no-ff
git push origin main
git branch -d refactor/ghost-edge-typed
```

---

### Branch 1.3: Scene Typed Collections ✅ COMPLETE
**Branch:** `refactor/scene-typed-collections` → **Status:** MERGED (commit: 43b4ccb)

**Implementation:**
- [x] scene.h: QHash<QUuid, Node*> m_nodes and QHash<QUuid, Edge*> m_edges (already existed)
- [x] scene.h: Added QList<Node*> selectedNodes() and QList<Edge*> selectedEdges() helpers
- [x] scene.cpp: Implemented typed selection helpers
- [x] window.cpp: Updated updateSelectionInfo() to use typed helpers

**Note:** Scene already had QHash-based typed collections from earlier work. This branch added selection helpers and cleaned up window.cpp. No new qgraphicsitem_cast violations eliminated (window.cpp was already clean). Auto-layout casts (scene.cpp:538, 641) were eliminated by Branch 2.4 (remove/unused-auto-layout).

---

### Branch 1.4: Delete Key Overhaul 🔥 CRITICAL
**Branch:** `refactor/delete-key-self-managed`

**Problem:** scene.cpp:454-458 loops selectedItems() casting to Node*/Edge* for deletion

**Changes:**
- [ ] node.h: Add `void keyPressEvent(QKeyEvent* event) override;`
- [ ] node.cpp: Implement - if Delete key, call `deleteLater()`, accept event
- [ ] edge.h: Add `void keyPressEvent(QKeyEvent* event) override;`
- [ ] edge.cpp: Implement - if Delete key, call `deleteLater()`, accept event
- [ ] scene.cpp: REMOVE Scene::keyPressEvent deletion logic (lines 435-480)

**Eliminates:** 3 cast violations
**Architectural win:** Objects manage their own lifecycle

**Git Workflow:**
```bash
git checkout main
git checkout -b refactor/ghost-edge-typed
# ... make changes ...
git add scene.h scene.cpp
git commit -m "refactor: replace ghost-edge casts with typed lookup

- add Scene::socketAt() helper using typed collections
- route ghost-edge hover/release through typed socket lookup
- keep magnet snapping + state reset paths on typed sockets
- eliminates final qgraphicsitem_cast occurrences"
git push origin refactor/ghost-edge-typed
git checkout main
git merge refactor/ghost-edge-typed --no-ff
git push origin main
git branch -d refactor/ghost-edge-typed
```

---

### Branch 1.5: Ghost Edge Typed Access
**Branch:** `refactor/ghost-edge-typed`

**Problem:** ghost-edge hover/release paths still depend on `itemAt()` + `qgraphicsitem_cast`

**Changes:**
- [ ] scene.h / scene.cpp: add `Socket* socketAt(const QPointF& scenePos) const` helper that iterates typed node/socket collections (optionally accept a snapping radius)
- [ ] scene.cpp: use `socketAt` inside `updateGhostEdge()` and `mouseReleaseEvent()`; remove direct `itemAt()` usage
- [ ] scene.cpp: keep magnet snapping and `resetAllSocketStates()` logic but ensure they rely exclusively on typed sockets
- [ ] Record a manual ghost-edge test checklist (below) covering drag, invalid targets, snapping, and cancel flows

**Test Plan:**
- [ ] Right-click drag from output to input creates an edge and clears highlights
- [ ] Drag to invalid targets (output->output, same node) turns ghost edge red and cancels cleanly
- [ ] Release near (not on) an input to confirm magnet snapping selects the closest valid socket
- [ ] Cancel mid-drag (Esc or secondary click) to verify the ghost edge and socket states reset

**Eliminates:** Final scene.cpp `qgraphicsitem_cast` usage (lines 326, 504)

**Git Workflow:**
```bash
git checkout main
git checkout -b refactor/ghost-edge-typed
# ... make changes ...
git add scene.h scene.cpp
git commit -m "refactor: replace ghost-edge casts with typed lookup

- add Scene::socketAt() helper using typed collections
- route ghost-edge hover/release through typed socket lookup
- keep magnet snapping + state reset paths on typed sockets
- eliminates final qgraphicsitem_cast occurrences"
git push origin refactor/ghost-edge-typed
git checkout main
git merge refactor/ghost-edge-typed --no-ff
git push origin main
git branch -d refactor/ghost-edge-typed
```

---

### Branch 1.6: Window Typed Queries
**Branch:** `refactor/window-typed-queries`

**Problem:** window.cpp remaining uses of scene item iteration with casts

**Changes:**
- [ ] window.cpp: Replace all scene->items() loops with Scene::nodes()/edges()
- [ ] window.cpp: Use typed collections for selection counting
- [ ] window.cpp: Remove any remaining qgraphicsitem_cast calls

**Eliminates:** Remaining window.cpp casts

**Git Workflow:**
```bash
git checkout main
git checkout -b refactor/ghost-edge-typed
# ... make changes ...
git add scene.h scene.cpp
git commit -m "refactor: replace ghost-edge casts with typed lookup

- add Scene::socketAt() helper using typed collections
- route ghost-edge hover/release through typed socket lookup
- keep magnet snapping + state reset paths on typed sockets
- eliminates final qgraphicsitem_cast occurrences"
git push origin refactor/ghost-edge-typed
git checkout main
git merge refactor/ghost-edge-typed --no-ff
git push origin main
git branch -d refactor/ghost-edge-typed
```

---

### Branch 1.7: Factory Typed Serialization
**Branch:** `refactor/factory-typed-serialization`

**Problem:** graph_factory.cpp:708 uses qgraphicsitem_cast for serialization

**Changes:**
- [ ] graph_factory.cpp: Use Node::inputSockets()/outputSockets() for serialization
- [ ] graph_factory.cpp: Remove childItems() casting
- [ ] Verify XML save/load still works

**Eliminates:** 1 cast violation (final one!)

**Git Workflow:**
```bash
git checkout main
git checkout -b refactor/ghost-edge-typed
# ... make changes ...
git add scene.h scene.cpp
git commit -m "refactor: replace ghost-edge casts with typed lookup

- add Scene::socketAt() helper using typed collections
- route ghost-edge hover/release through typed socket lookup
- keep magnet snapping + state reset paths on typed sockets
- eliminates final qgraphicsitem_cast occurrences"
git push origin refactor/ghost-edge-typed
git checkout main
git merge refactor/ghost-edge-typed --no-ff
git push origin main
git branch -d refactor/ghost-edge-typed
```

---

## Phase 2: Critical Bug Fixes (Parallel to Phase 1)

**Note:** Bugs identified by Codex (from ISSUE.md analysis). Can be done in parallel with architecture work.

### Branch 2.1: Fix Graph::saveToFile() 🔥
**Branch:** `fix/graph-save-to-file`

**Problem:** `graph.cpp:371-379` - Graph::saveToFile() returns true but never writes file

**Changes:**
- [ ] `graph.cpp:371-379` - Wire to `m_factory->saveToXmlFile(filePath)`
- [ ] Remove TODO comment
- [ ] Add error handling for save failures

**Implementation:**
```cpp
bool Graph::saveToFile(const QString& filePath)
{
    qDebug() << "Graph::saveToFile:" << filePath;

    if (!m_factory) {
        qWarning() << "Cannot save: No factory available";
        return false;
    }

    bool success = m_factory->saveToXmlFile(filePath);
    if (success) {
        emit graphSaved(filePath);
    }
    return success;
}
```

**Test Plan:**
- [ ] Build and run application
- [ ] Create test graph with 3 nodes, 2 edges
- [ ] Call `graph.saveToFile("test_save.xml")` from JavaScript CLI
- [ ] Verify file exists in filesystem
- [ ] Verify file size > 0
- [ ] Load file via `graph.loadFromFile("test_save.xml")`
- [ ] Verify graph restored correctly

**Git Workflow:**
```bash
git checkout main
git checkout -b refactor/ghost-edge-typed
# ... make changes ...
git add scene.h scene.cpp
git commit -m "refactor: replace ghost-edge casts with typed lookup

- add Scene::socketAt() helper using typed collections
- route ghost-edge hover/release through typed socket lookup
- keep magnet snapping + state reset paths on typed sockets
- eliminates final qgraphicsitem_cast occurrences"
git push origin refactor/ghost-edge-typed
git checkout main
git merge refactor/ghost-edge-typed --no-ff
git push origin main
git branch -d refactor/ghost-edge-typed
```

---

### Branch 2.2: Fix Node Position Precision
**Branch:** `fix/node-position-precision`

**Problem:** `node.cpp:123` only updates m_lastPos if movement > 5px, but `node.cpp:434-435` saves m_lastPos. Small nudges lost on save/reload.

**Changes:**
- [ ] `node.cpp:434-435` - Serialize `pos()` instead of `m_lastPos`

**Implementation:**
```cpp
// node.cpp:434-435 - Change from:
xmlSetProp(node, BAD_CAST "x", BAD_CAST QString::number(m_lastPos.x()).toUtf8().constData());
xmlSetProp(node, BAD_CAST "y", BAD_CAST QString::number(m_lastPos.y()).toUtf8().constData());

// To:
QPointF currentPos = pos();
xmlSetProp(node, BAD_CAST "x", BAD_CAST QString::number(currentPos.x()).toUtf8().constData());
xmlSetProp(node, BAD_CAST "y", BAD_CAST QString::number(currentPos.y()).toUtf8().constData());
```

**Test Plan:**
- [ ] Build and run application
- [ ] Create node at (100, 100)
- [ ] Nudge node 2px to (102, 100)
- [ ] Save graph
- [ ] Clear graph
- [ ] Load graph
- [ ] Verify node at (102, 100) not (100, 100)

**Git Workflow:**
```bash
git checkout main
git checkout -b refactor/ghost-edge-typed
# ... make changes ...
git add scene.h scene.cpp
git commit -m "refactor: replace ghost-edge casts with typed lookup

- add Scene::socketAt() helper using typed collections
- route ghost-edge hover/release through typed socket lookup
- keep magnet snapping + state reset paths on typed sockets
- eliminates final qgraphicsitem_cast occurrences"
git push origin refactor/ghost-edge-typed
git checkout main
git merge refactor/ghost-edge-typed --no-ff
git push origin main
git branch -d refactor/ghost-edge-typed
```

---

### Branch 2.3: Fix Load Rollback on Validation Failure ✅ COMPLETE
**Branch:** `fix/load-rollback` → **Status:** MERGED (commit: fix/load-rollback-memory-leak)

**Problem:** `graph_factory.cpp:461-535` created nodes/edges in Phase 2, but Phase 3 validation could fail (duplicate sockets). On failure, objects allocated but not deleted → memory leak.

**Implementation:**
- [x] Added cleanup loops to 4 error paths in graph_factory.cpp
- [x] Lines 463-466: Delete allNodes on node creation failure
- [x] Lines 486-493: Delete allNodes + allEdges on edge creation failure
- [x] Lines 530-535: Delete allNodes + allEdges on duplicate output socket validation failure
- [x] Lines 548-553: Delete allNodes + allEdges on duplicate input socket validation failure
- [x] Each cleanup loop explicitly calls `delete` before `return false`

**Validation:**
- [x] Created test_corrupt_duplicate.xml with duplicate socket connections
- [x] Created test_memory_leak.js to load corrupt file 50 times
- [x] Added ASAN support to CMakeLists.txt (Linux/WSL Debug builds)
- [x] Created asan_suppressions.txt for OpenGL false positives
- [x] Ran ASAN test on WSL: NO leaks from graph_factory.cpp ✓
- [x] Updated ISSUE.md to document the fix

**Key Learning:** Manual `delete` is correct pattern for Qt graphics items. Smart pointers (std::unique_ptr) would conflict with QGraphicsScene's parent-child ownership system.

---

### Branch 2.4: Remove Unused Auto-Layout Code
**Branch:** `remove/unused-auto-layout`

**Problem:** Scene contains two unused auto-layout functions that don't work and should be removed entirely. These functions also contain 2 qgraphicsitem_cast violations.

**Functions to Remove:**
- `Scene::autoLayoutAnneal` (scene.cpp:526) - attempted simulated annealing, doesn't work
- `Scene::autoLayoutForceDirected` (scene.cpp:629) - attempted force-directed layout, doesn't work

**Changes:**
- [ ] scene.h: Remove method declarations for both functions
- [ ] scene.cpp: Delete both function implementations
- [ ] window.cpp: Remove menu items/shortcuts that call these functions (if any)
- [ ] Verify no other callers exist with `git grep "autoLayout"`

**Eliminates:** 2 qgraphicsitem_cast violations (scene.cpp:532, 635)

**Test Plan:**
- [ ] Build and run application
- [ ] Verify no compilation errors
- [ ] Verify no runtime crashes
- [ ] Verify menu/shortcuts still work for remaining features

**Git Workflow:**
```bash
git checkout main
git checkout -b refactor/ghost-edge-typed
# ... make changes ...
git add scene.h scene.cpp
git commit -m "refactor: replace ghost-edge casts with typed lookup

- add Scene::socketAt() helper using typed collections
- route ghost-edge hover/release through typed socket lookup
- keep magnet snapping + state reset paths on typed sockets
- eliminates final qgraphicsitem_cast occurrences"
git push origin refactor/ghost-edge-typed
git checkout main
git merge refactor/ghost-edge-typed --no-ff
git push origin main
git branch -d refactor/ghost-edge-typed
```

---

### Branch 2.5: Enhanced Edge Visual Ordering
**Branch:** `feature/edge-z-order-enhancements`

**Status:** Basic implementation MERGED (commit: feature/edge-visual-ordering)
**Current Implementation:**
- [x] Selection z-order boost: selected edges jump to z=10
- [x] Connectivity-based stacking: edges with more connections get higher z-values (z = 2.0 + connections * 0.02)

**Remaining Improvements:**
- [ ] Cache z-value before selection to restore properly (currently always restores to z=2)
- [ ] Trigger `updateZOrderFromConnections()` when node connectivity changes (add/remove edges)
- [ ] Add perpendicular offset to control points for parallel edges between same nodes (fan-out effect)
- [ ] Add smoke test for deterministic z-ordering

**Changes Needed:**

**edge.h:**
```cpp
private:
    qreal m_normalZValue;  // Cache z-value before selection
```

**edge.cpp:**
```cpp
// In itemChange():
if (change == ItemSelectedHasChanged) {
    if (value.toBool()) {
        m_normalZValue = zValue();  // Cache before boosting
        setZValue(10);
    } else {
        setZValue(m_normalZValue);  // Restore cached value
    }
}

// Call updateZOrderFromConnections() after edge creation/deletion
// Add to Node::addEdge() and Node::removeEdge() if they exist
```

**scene.cpp:**
```cpp
// After creating edge in connectSockets():
edge->updateZOrderFromConnections();

// After deleting edge:
// Update z-order of remaining edges connected to affected nodes
```

**Test Plan:**
- [ ] Load tests_large.xml with MERGE nodes
- [ ] Verify edges stack by connectivity
- [ ] Select edge, verify it comes to front at z=10
- [ ] Deselect edge, verify it returns to correct stacked z-value (not always z=2)
- [ ] Add new edge, verify all connected edges update their z-order
- [ ] Delete edge, verify remaining edges update their z-order

**Git Workflow:**
```bash
git checkout main
git checkout -b refactor/ghost-edge-typed
# ... make changes ...
git add scene.h scene.cpp
git commit -m "refactor: replace ghost-edge casts with typed lookup

- add Scene::socketAt() helper using typed collections
- route ghost-edge hover/release through typed socket lookup
- keep magnet snapping + state reset paths on typed sockets
- eliminates final qgraphicsitem_cast occurrences"
git push origin refactor/ghost-edge-typed
git checkout main
git merge refactor/ghost-edge-typed --no-ff
git push origin main
git branch -d refactor/ghost-edge-typed
```

---

### Branch 2.6: Improve View/Scene Spacing and Navigation
**Branch:** `feature/improved-view-navigation`

**Problem:** Multiple viewport and navigation issues
- **Fixed scene rect:** scene.cpp:32 has fixed 2000x2000 area, large graphs go outside bounds
- **No auto-center:** Loading XML doesn't center view on content
- **No panning:** setDragMode(NoDrag) prevents middle-click pan
- **No auto-fit:** Can't zoom to fit entire graph
- **Tight node spacing:** 100px spacing causes overlaps (nodes are ~120-150px wide)

**Changes:**

**scene.cpp:**
- [ ] Replace fixed sceneRect with dynamic calculation based on content
- [ ] Add `updateSceneRect()` method that expands to fit all nodes + margin
- [ ] Call updateSceneRect() after loading graph, adding nodes, moving nodes

**view.cpp:**
- [ ] Change `setDragMode(QGraphicsView::ScrollHandDrag)` for middle-click pan
- [ ] Add Ctrl+0 shortcut to fit entire graph in view
- [ ] Add `centerOnGraph()` method to center view on content bounding box

**window.cpp:**
- [ ] Call `view->centerOnGraph()` after loading XML files
- [ ] Add menu item "View > Fit Graph" (Ctrl+0)
- [ ] Add constants for node spacing (HORIZONTAL_SPACING = 300, VERTICAL_SPACING = 180)
- [ ] Update node creation positions to use proper spacing

**generate_test_files.py (for testing):**
- [ ] Update spacing from 100px to 300px horizontal, 180px vertical
- [ ] Reduce jitter from ±20 to ±10 for cleaner layouts
- [ ] Fix UUID format: remove braces (use str(uuid.uuid4()) not "{...}")

**Test Plan:**
- [ ] Generate new test files with improved spacing
- [ ] Load tests_tiny.xml, tests_small.xml, tests_medium.xml
- [ ] Verify nodes don't overlap
- [ ] Verify edges have clear routing space
- [ ] Test with different node types (MERGE, SPLIT have more sockets)

**Git Workflow:**
```bash
git checkout main
git checkout -b refactor/ghost-edge-typed
# ... make changes ...
git add scene.h scene.cpp
git commit -m "refactor: replace ghost-edge casts with typed lookup

- add Scene::socketAt() helper using typed collections
- route ghost-edge hover/release through typed socket lookup
- keep magnet snapping + state reset paths on typed sockets
- eliminates final qgraphicsitem_cast occurrences"
git push origin refactor/ghost-edge-typed
git checkout main
git merge refactor/ghost-edge-typed --no-ff
git push origin main
git branch -d refactor/ghost-edge-typed
```

---

## Phase 3: Payload Infrastructure (Data Layer)

### Branch 3.1: Add Payload Storage to Node and Edge
**Branch:** `feature/node-edge-payload-storage`

**Purpose:** Add QVariantMap-based payload storage to nodes/edges with XML persistence

**Changes:**

**node.h:**
- [ ] Add `QVariantMap m_payload;` private member
- [ ] Add payload API methods
- [ ] Add serialization methods

**node.cpp:**
- [ ] Implement payload getters/setters
- [ ] Implement writePayload()/readPayload() for XML
- [ ] Wire into Node::write() and Node::read()

**edge.h / edge.cpp:**
- [ ] Same payload infrastructure as Node

**Test Plan:**
- [ ] Create node, set payload, save, reload
- [ ] Verify payload persists through XML round-trip
- [ ] Test multiple types: string, int, double, bool

**Git Workflow:**
```bash
git checkout main
git checkout -b refactor/ghost-edge-typed
# ... make changes ...
git add scene.h scene.cpp
git commit -m "refactor: replace ghost-edge casts with typed lookup

- add Scene::socketAt() helper using typed collections
- route ghost-edge hover/release through typed socket lookup
- keep magnet snapping + state reset paths on typed sockets
- eliminates final qgraphicsitem_cast occurrences"
git push origin refactor/ghost-edge-typed
git checkout main
git merge refactor/ghost-edge-typed --no-ff
git push origin main
git branch -d refactor/ghost-edge-typed
```

---

### Branch 3.2: Expose Payload API through Graph Facade
**Branch:** `feature/payload-facade-api`

**Purpose:** Make payload operations callable from JavaScript via Graph facade

**Changes:**

**graph.h:**
- [ ] Add Q_INVOKABLE setNodePayload/getNodePayload/getNodeAllPayload
- [ ] Add Q_INVOKABLE setEdgePayload/getEdgePayload/getEdgeAllPayload
- [ ] Add signals: nodePayloadChanged, edgePayloadChanged

**graph.cpp:**
- [ ] Implement node/edge payload methods using Scene::findNodeById/findEdgeById
- [ ] Emit signals on payload changes
- [ ] Trigger autosave on mutations

**Test Plan:**
- [ ] Test from JavaScript CLI
- [ ] Set/get payloads via facade
- [ ] Verify save/load persistence
- [ ] Verify autosave triggers

**Git Workflow:**
```bash
git checkout main
git checkout -b refactor/ghost-edge-typed
# ... make changes ...
git add scene.h scene.cpp
git commit -m "refactor: replace ghost-edge casts with typed lookup

- add Scene::socketAt() helper using typed collections
- route ghost-edge hover/release through typed socket lookup
- keep magnet snapping + state reset paths on typed sockets
- eliminates final qgraphicsitem_cast occurrences"
git push origin refactor/ghost-edge-typed
git checkout main
git merge refactor/ghost-edge-typed --no-ff
git push origin main
git branch -d refactor/ghost-edge-typed
```

---

## Phase 4: JavaScript Scripting Layer

### Branch 4.1: Node JavaScript Behavior Execution
**Branch:** `feature/node-javascript-behavior`

**Purpose:** Allow nodes to execute custom JavaScript code with access to their own context

**Design:**
- Nodes store JavaScript source as special payload key: `"__jsBehavior"`
- Graph facade passes its QJSEngine reference to nodes for execution
- Nodes get `node` object in JS context with id, type, payload

**Changes:**

**node.h:**
- [ ] Add setJavaScriptBehavior/getJavaScriptBehavior/executeJavaScript methods

**node.cpp:**
- [ ] Implement JavaScript execution with context injection
- [ ] Expose node object (id, type, payload, position) to scripts

**graph.h:**
- [ ] Add Q_INVOKABLE setNodeBehavior/getNodeBehavior/executeNodeScript

**graph.cpp:**
- [ ] Implement facade methods calling Node::executeJavaScript with m_jsEngine

**Test Plan:**
- [ ] Create test_node_scripting.js
- [ ] Set node behavior from JavaScript
- [ ] Execute scripts with context parameters
- [ ] Verify scripts persist through save/load
- [ ] Test error handling for bad scripts

**Git Workflow:**
```bash
git checkout main
git checkout -b refactor/ghost-edge-typed
# ... make changes ...
git add scene.h scene.cpp
git commit -m "refactor: replace ghost-edge casts with typed lookup

- add Scene::socketAt() helper using typed collections
- route ghost-edge hover/release through typed socket lookup
- keep magnet snapping + state reset paths on typed sockets
- eliminates final qgraphicsitem_cast occurrences"
git push origin refactor/ghost-edge-typed
git checkout main
git merge refactor/ghost-edge-typed --no-ff
git push origin main
git branch -d refactor/ghost-edge-typed
```

---

### Branch 4.2: Edge JavaScript Expressions
**Branch:** `feature/edge-javascript-expressions`

**Purpose:** Edges can evaluate JavaScript expressions with access to connected nodes

**Use Case:** Dynamic edge weights based on node properties

**Changes:**

**edge.h:**
- [ ] Add setExpression/getExpression/evaluateExpression methods

**edge.cpp:**
- [ ] Implement expression evaluation
- [ ] Expose fromNode, toNode, edge objects to expressions

**graph.h:**
- [ ] Add Q_INVOKABLE setEdgeExpression/getEdgeExpression/evaluateEdge

**graph.cpp:**
- [ ] Implement facade methods

**Test Plan:**
- [ ] Create nodes with weights
- [ ] Set edge expression: `fromNode.payload.weight + toNode.payload.weight`
- [ ] Verify evaluation returns correct sum
- [ ] Update node payload, verify re-evaluation works

**Git Workflow:**
```bash
git checkout main
git checkout -b refactor/ghost-edge-typed
# ... make changes ...
git add scene.h scene.cpp
git commit -m "refactor: replace ghost-edge casts with typed lookup

- add Scene::socketAt() helper using typed collections
- route ghost-edge hover/release through typed socket lookup
- keep magnet snapping + state reset paths on typed sockets
- eliminates final qgraphicsitem_cast occurrences"
git push origin refactor/ghost-edge-typed
git checkout main
git merge refactor/ghost-edge-typed --no-ff
git push origin main
git branch -d refactor/ghost-edge-typed
```

---

## Phase 5: Template Integration

### Branch 5.1: Default Payloads and Scripts in Templates
**Branch:** `feature/template-default-payloads`

**Purpose:** NodeTypeTemplates can specify default payloads and JavaScript behaviors

**Changes:**

**node_type_templates.h:**
- [ ] Update registerTemplate to accept defaultPayload parameter

**node_type_templates.cpp:**
- [ ] Define default behaviors for SOURCE, TRANSFORM, SINK
- [ ] Apply default payloads on node creation

**Test Plan:**
- [ ] Create SOURCE node, verify default payload/behavior applied
- [ ] Execute default behavior, verify it works
- [ ] Override default, verify custom behavior takes precedence

**Git Workflow:**
```bash
git checkout main
git checkout -b refactor/ghost-edge-typed
# ... make changes ...
git add scene.h scene.cpp
git commit -m "refactor: replace ghost-edge casts with typed lookup

- add Scene::socketAt() helper using typed collections
- route ghost-edge hover/release through typed socket lookup
- keep magnet snapping + state reset paths on typed sockets
- eliminates final qgraphicsitem_cast occurrences"
git push origin refactor/ghost-edge-typed
git checkout main
git merge refactor/ghost-edge-typed --no-ff
git push origin main
git branch -d refactor/ghost-edge-typed
```

---

## Phase 6: Examples and Documentation

### Branch 6.1: JavaScript Integration Examples
**Branch:** `test/javascript-integration-examples`

**Purpose:** Create comprehensive examples demonstrating JavaScript integration

**Changes:**
- [ ] Create examples/ directory
- [ ] examples/01_basic_payload.js
- [ ] examples/02_node_scripting.js
- [ ] examples/03_edge_expressions.js
- [ ] examples/04_data_pipeline.js
- [ ] Create JAVASCRIPT_API.md documenting all facade methods
- [ ] Update README.md with JavaScript integration section

**Git Workflow:**
```bash
git checkout main
git checkout -b refactor/ghost-edge-typed
# ... make changes ...
git add scene.h scene.cpp
git commit -m "refactor: replace ghost-edge casts with typed lookup

- add Scene::socketAt() helper using typed collections
- route ghost-edge hover/release through typed socket lookup
- keep magnet snapping + state reset paths on typed sockets
- eliminates final qgraphicsitem_cast occurrences"
git push origin refactor/ghost-edge-typed
git checkout main
git merge refactor/ghost-edge-typed --no-ff
git push origin main
git branch -d refactor/ghost-edge-typed
```

---

## Progress Tracking

### Phase 1: Architecture (qgraphicsitem_cast Elimination) 🔥 CRITICAL
- [x] Branch 1.1: refactor/socket-typed-parent (commit: 489305e) - 1 cast eliminated
- [x] Branch 1.2: refactor/node-typed-collections (commit: 8975361) - 2 casts eliminated
- [x] Branch 1.3: refactor/scene-typed-collections (commit: 43b4ccb) - infrastructure, 0 casts (already clean)
- [ ] Branch 1.4: refactor/delete-key-self-managed 🔥 (SKIP - risky, not required for final goal)
- [ ] **Branch 1.5: refactor/ghost-edge-typed** ← NEXT - eliminates 2 remaining casts (scene.cpp:326, 504)
- [ ] Branch 1.6: refactor/window-typed-queries (likely already done)
- [x] Branch 1.7: refactor/factory-typed-serialization (commit: 542a5e8, 128a1b3) - 1 cast eliminated + VS debugger fix

### Phase 2: Bug Fixes (Parallel to Phase 1)
- [ ] Branch 2.1: fix/graph-save-to-file
- [ ] Branch 2.2: fix/node-position-precision
- [x] Branch 2.3: fix/load-rollback (commit: fix/load-rollback-memory-leak) - ASAN validated ✓
- [x] Branch 2.4: remove/unused-auto-layout (commit: 8f0ad98) - 2 casts eliminated, 306 lines removed
- [x] Branch 2.5: feature/edge-z-order-enhancements (basic implementation, commit: 2b7bb36)

### Phase 3: Payload Infrastructure
- [ ] Branch 3.1: feature/node-edge-payload-storage
- [ ] Branch 3.2: feature/payload-facade-api

### Phase 4: JavaScript Scripting
- [ ] Branch 4.1: feature/node-javascript-behavior
- [ ] Branch 4.2: feature/edge-javascript-expressions

### Phase 5: Template Integration
- [ ] Branch 5.1: feature/template-default-payloads

### Phase 6: Examples & Documentation
- [ ] Branch 6.1: test/javascript-integration-examples

---

## Success Criteria

**JavaScript integration is complete when:**
- [x] Graph facade exists with QJSEngine (DONE)
- [ ] Nodes can execute custom JavaScript
- [ ] Edges can evaluate expressions
- [ ] Payloads persist through save/load
- [ ] All APIs callable from `--script`
- [ ] Comprehensive examples work
- [ ] Documentation complete

**Final Vision:**
```javascript
// Create fully programmable graph from JavaScript
var source = graph.createNode("SOURCE", 0, 0, 1);
graph.setNodeBehavior(source, `
    this.value = (this.value || 0) + 1;
    return this.value;
`);

var transform = graph.createNode("TRANSFORM", 200, 0, 1);
graph.setNodePayload(transform, "multiplier", 2);
graph.setNodeBehavior(transform, `
    return input * node.payload.multiplier;
`);

var edge = graph.connectNodes(source, 0, transform, 0);
graph.setEdgeExpression(edge, `
    fromNode.payload.weight || 1.0
`);

// Execute graph
for (var i = 0; i < 5; i++) {
    var val = graph.executeNodeScript(source);
    var result = graph.executeNodeScript(transform, {input: val});
    console.log("Step", i, ":", val, "->", result);
}

// Save entire scripted graph
graph.saveToFile("my_program.xml");
```

---

## Deferred Low-Priority Items

**From ISSUE.md (Codex analysis):**

1. **Harmonize Template Names**
   - [ ] Audit callers of GraphFactory::createNode for wrong type strings
   - [ ] Likely already fixed - wrong names were only in removed test code
   - [ ] Verify palette and shortcuts use correct uppercase names

2. **Tame Diagnostic Noise**
   - [ ] Create NG_VERBOSE environment variable for logging control
   - [ ] Route qDebug() calls through logging helper
   - [ ] Replace QMessageBox notifications with status bar updates
   - Low priority - current logging useful for debugging

---

## Operational Rules

1. **Build/test after every branch** - Never merge broken code
2. **Commit frequently** - After each logical change
3. **Tag milestones** - After each phase complete
4. **One feature per branch** - Keep changes focused
5. **Always test from JavaScript** - Use `--script` to verify APIs
6. **Update this plan** - Mark [x] when complete, add notes

---

**Next Step:** Start Phase 1, Branch 1.1 - Socket Typed Parent (eliminate qgraphicsitem_cast)

