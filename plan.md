# NodeGraph Development Plan
**Updated:** 2025-01-29
**PRIMARY GOAL:** Eliminate qgraphicsitem_cast - architectural rot
**SECONDARY GOAL:** Full JavaScript integration for programmable nodes/edges

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
git checkout -b refactor/socket-typed-parent
# ... make changes ...
git add socket.h socket.cpp node.cpp
git commit -m "refactor: add typed parent pointer to Socket

- Add Node* m_parentNode member
- Initialize in constructor
- Remove qgraphicsitem_cast from socket.cpp:42
- Eliminates 1/17 cast violations"
git push origin refactor/socket-typed-parent
git checkout main
git merge refactor/socket-typed-parent --no-ff
git push origin main
git branch -d refactor/socket-typed-parent
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
git checkout -b refactor/node-typed-collections
# ... make changes ...
git add node.h node.cpp
git commit -m "refactor: add typed Socket collections to Node

- Add QList<Socket*> for inputs/outputs
- Store sockets in lists on creation
- Remove childItems() + qgraphicsitem_cast loops
- Eliminates 2/17 cast violations (node.cpp:188, 426)"
git push origin refactor/node-typed-collections
git checkout main
git merge refactor/node-typed-collections --no-ff
git push origin main
git branch -d refactor/node-typed-collections
```

---

### Branch 1.3: Scene Typed Collections
**Branch:** `refactor/scene-typed-collections`

**Problem:** scene.cpp:538, 641 + window.cpp:895, 897 iterate items() with casts

**Changes:**
- [ ] scene.h: Add `QList<Node*> m_nodes` and `QList<Edge*> m_edges`
- [ ] scene.h: Add `QList<Node*> nodes()` and `QList<Edge*> edges()` accessors
- [ ] scene.cpp: Add nodes/edges to typed lists when creating
- [ ] scene.cpp: Remove items() iteration with casts
- [ ] window.cpp: Use Scene::nodes() and Scene::edges() instead of casting

**Eliminates:** 4 cast violations

**Git Workflow:**
```bash
git checkout main
git checkout -b refactor/scene-typed-collections
# ... make changes ...
git add scene.h scene.cpp window.cpp
git commit -m "refactor: add typed Node/Edge collections to Scene

- Add QList<Node*> m_nodes and QList<Edge*> m_edges
- Track nodes/edges on creation
- Remove items() + qgraphicsitem_cast loops
- Update window.cpp to use typed accessors
- Eliminates 4/17 cast violations"
git push origin refactor/scene-typed-collections
git checkout main
git merge refactor/scene-typed-collections --no-ff
git push origin main
git branch -d refactor/scene-typed-collections
```

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
git checkout -b refactor/delete-key-self-managed
# ... make changes ...
git add node.h node.cpp edge.h edge.cpp scene.cpp
git commit -m "refactor: move Delete key handling to items

- Add keyPressEvent() to Node and Edge
- Items delete themselves via deleteLater()
- Remove scene-level deletion with type checking
- Objects manage own lifecycle
- Eliminates 3/17 cast violations (scene.cpp:454-458)"
git push origin refactor/delete-key-self-managed
git checkout main
git merge refactor/delete-key-self-managed --no-ff
git push origin main
git branch -d refactor/delete-key-self-managed
```

---

### Branch 1.5: Ghost Edge Typed Access
**Branch:** `refactor/ghost-edge-typed`

**Problem:** scene.cpp ghost edge code uses casts to find sockets

**Changes:**
- [ ] node.h: Add `Socket* socketAtPoint(const QPointF& scenePos) const`
- [ ] node.cpp: Implement using typed m_inputSockets/m_outputSockets
- [ ] scene.cpp: Use Node::socketAtPoint() instead of casting childItems()
- [ ] socket.cpp:183: Complete TODO - left-click starts ghost edge
- [ ] socket.cpp:204: Complete TODO - left-click completes connection

**Eliminates:** Remaining scene.cpp ghost edge casts

**Git Workflow:**
```bash
git checkout main
git checkout -b refactor/ghost-edge-typed
# ... make changes ...
git add node.h node.cpp scene.cpp socket.cpp
git commit -m "refactor: use typed accessors for ghost edge

- Add Node::socketAtPoint() using typed socket lists
- Remove scene childItems() casting for ghost edges
- Complete left-click ghost edge drag TODOs
- Eliminates ghost edge cast violations"
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
git checkout -b refactor/window-typed-queries
# ... make changes ...
git add window.cpp
git commit -m "refactor: use typed queries in window.cpp

- Replace items() loops with nodes()/edges()
- Use typed collections for all queries
- Eliminates remaining window.cpp casts"
git push origin refactor/window-typed-queries
git checkout main
git merge refactor/window-typed-queries --no-ff
git push origin main
git branch -d refactor/window-typed-queries
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
git checkout -b refactor/factory-typed-serialization
# ... make changes ...
git add graph_factory.cpp
git commit -m "refactor: use typed socket accessors in factory

- Use Node::inputSockets()/outputSockets() for XML serialization
- Remove final qgraphicsitem_cast violation
- Eliminates 1/17 cast violations (graph_factory.cpp:708)

🎉 MILESTONE: ZERO qgraphicsitem_cast in entire codebase"
git push origin refactor/factory-typed-serialization
git checkout main
git merge refactor/factory-typed-serialization --no-ff
git push origin main
git branch -d refactor/factory-typed-serialization
git tag -a v1.0-zero-cast -m "Architecture: Zero qgraphicsitem_cast"
git push origin v1.0-zero-cast
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
git checkout -b fix/graph-save-to-file
# ... make changes ...
git add graph.cpp graph.h
git commit -m "fix: implement Graph::saveToFile() - wire to GraphFactory

- Connect Graph::saveToFile() to m_factory->saveToXmlFile()
- Add null factory check with error logging
- Emit graphSaved signal only on success
- Fixes JavaScript saveToFile() returning true but not writing files"
git push origin fix/graph-save-to-file
git checkout main
git merge fix/graph-save-to-file --no-ff
git push origin main
git branch -d fix/graph-save-to-file
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
git checkout -b fix/node-position-precision
# ... make changes ...
git add node.cpp
git commit -m "fix: preserve exact node positions on save

- Serialize pos() instead of m_lastPos
- Fixes small movements (<5px) being lost on save/reload
- Ensures WYSIWYG for node positions"
git push origin fix/node-position-precision
git checkout main
git merge fix/node-position-precision --no-ff
git push origin main
git branch -d fix/node-position-precision
```

---

### Branch 2.3: Fix Load Rollback on Validation Failure
**Branch:** `fix/load-rollback`

**Problem:** `graph_factory.cpp:461-535` creates nodes/edges in Phase 2, but Phase 3 validation can fail (duplicate sockets). On failure, scene left with orphaned objects.

**Changes:**
- [ ] `graph_factory.cpp:461-491` - Defer scene insertion until Phase 3 passes
- [ ] Store created nodes/edges in temporary vectors
- [ ] On Phase 3 validation failure: delete objects and call `GraphSubject::endBatch()`
- [ ] Ensure `endBatch()` always paired with `beginBatch()`

**Implementation Strategy:**
1. Create nodes/edges but DON'T add to scene yet
2. Store in local vectors: `QVector<Node*> pendingNodes`, `QVector<Edge*> pendingEdges`
3. Run Phase 3 validation (socket duplicate check)
4. If validation passes: add to scene, emit signals
5. If validation fails: delete all pending objects, `endBatch()`, return false

**Test Plan:**
- [ ] Create malformed XML file with duplicate socket connections
- [ ] Build and run application
- [ ] Clear graph, count nodes/edges (should be 0/0)
- [ ] Load malformed file
- [ ] Verify error message about duplicate socket
- [ ] Verify loadFromXmlFile() returns false
- [ ] Count nodes/edges (should still be 0/0 - no orphans)
- [ ] Load valid file, verify success

**Git Workflow:**
```bash
git checkout main
git checkout -b fix/load-rollback
# ... make changes ...
git add graph_factory.cpp
git commit -m "fix: guarantee scene rollback on XML validation failure

- Defer scene insertion until Phase 3 validation passes
- Store nodes/edges in pending vectors during Phase 2
- Delete pending objects if Phase 3 fails (duplicate sockets)
- Ensure GraphSubject::endBatch() always called
- Prevents orphaned objects in scene on load failure"
git push origin fix/load-rollback
git checkout main
git merge fix/load-rollback --no-ff
git push origin main
git branch -d fix/load-rollback
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
git checkout -b feature/node-edge-payload-storage
# ... make changes ...
git add node.h node.cpp edge.h edge.cpp
git commit -m "feat: add payload storage to Node and Edge classes

- Add QVariantMap m_payload to Node and Edge
- Implement setPayload/getPayload/getAllPayload/clearPayload API
- Add XML serialization via writePayload/readPayload methods
- Support string, int, double, bool types
- Payloads persist through save/load cycle
- Foundation for JavaScript property access"
git push origin feature/node-edge-payload-storage
git checkout main
git merge feature/node-edge-payload-storage --no-ff
git push origin main
git branch -d feature/node-edge-payload-storage
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
git checkout -b feature/payload-facade-api
# ... make changes ...
git add graph.h graph.cpp
git commit -m "feat: expose payload API through Graph facade

- Add Q_INVOKABLE payload methods for nodes and edges
- Emit nodePayloadChanged/edgePayloadChanged signals
- Trigger autosave on payload mutations
- Enables JavaScript access to node/edge payloads
- Foundation for scriptable node properties"
git push origin feature/payload-facade-api
git checkout main
git merge feature/payload-facade-api --no-ff
git push origin main
git branch -d feature/payload-facade-api
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
git checkout -b feature/node-javascript-behavior
# ... make changes ...
git add node.h node.cpp graph.h graph.cpp
git commit -m "feat: enable JavaScript behavior execution in nodes

- Add Node::executeJavaScript() with context injection
- Store JS source in payload['__jsBehavior']
- Expose 'node' object to scripts (id, type, payload, position)
- Add Graph::setNodeBehavior/getNodeBehavior/executeNodeScript
- Scripts persist through save/load cycle
- Foundation for programmable node logic"
git push origin feature/node-javascript-behavior
git checkout main
git merge feature/node-javascript-behavior --no-ff
git push origin main
git branch -d feature/node-javascript-behavior
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
git checkout -b feature/edge-javascript-expressions
# ... make changes ...
git add edge.h edge.cpp graph.h graph.cpp
git commit -m "feat: enable JavaScript expression evaluation in edges

- Add Edge::evaluateExpression() with node context
- Store expression in payload['__jsExpression']
- Expose fromNode, toNode, edge objects to expressions
- Add Graph::setEdgeExpression/getEdgeExpression/evaluateEdge
- Enables dynamic edge weights based on connected nodes
- Expressions persist through save/load"
git push origin feature/edge-javascript-expressions
git checkout main
git merge feature/edge-javascript-expressions --no-ff
git push origin main
git branch -d feature/edge-javascript-expressions
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
git checkout -b feature/template-default-payloads
# ... make changes ...
git add node_type_templates.h node_type_templates.cpp
git commit -m "feat: add default payloads and scripts to node templates

- Update registerTemplate() to accept defaultPayload
- Define default behaviors for SOURCE, TRANSFORM, SINK
- Apply default payloads on node creation
- Enables out-of-box scripted node functionality
- Users can override defaults as needed"
git push origin feature/template-default-payloads
git checkout main
git merge feature/template-default-payloads --no-ff
git push origin main
git branch -d feature/template-default-payloads
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
git checkout -b test/javascript-integration-examples
# ... create example files ...
git add examples/ JAVASCRIPT_API.md README.md
git commit -m "docs: add comprehensive JavaScript integration examples

- Create examples/ directory with demonstration scripts
- Document all Graph facade JavaScript API methods
- Show payload, scripting, and expression capabilities
- Provide data pipeline examples
- Update README with JavaScript integration section"
git push origin test/javascript-integration-examples
git checkout main
git merge test/javascript-integration-examples --no-ff
git push origin main
git branch -d test/javascript-integration-examples
```

---

## Progress Tracking

### Phase 1: Architecture (qgraphicsitem_cast Elimination) 🔥 CRITICAL
- [x] Branch 1.1: refactor/socket-typed-parent (commit: 489305e)
- [ ] Branch 1.2: refactor/node-typed-collections
- [ ] Branch 1.3: refactor/scene-typed-collections
- [ ] Branch 1.4: refactor/delete-key-self-managed 🔥
- [ ] Branch 1.5: refactor/ghost-edge-typed
- [ ] Branch 1.6: refactor/window-typed-queries
- [ ] Branch 1.7: refactor/factory-typed-serialization

### Phase 2: Bug Fixes (Parallel to Phase 1)
- [ ] Branch 2.1: fix/graph-save-to-file
- [ ] Branch 2.2: fix/node-position-precision
- [ ] Branch 2.3: fix/load-rollback

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
