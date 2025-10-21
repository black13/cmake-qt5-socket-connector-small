# Plan Overview (agreed with plan_response.md)

We accept the architectural direction set out in `plan_response.md`:
- ✅ Delete handling must live in `Node::keyPressEvent` / `Edge::keyPressEvent` with the scene-wide loop removed.
- ✅ The stub-based JavaScript approach stays; the legacy `javascript_console.*` files are gone.
- ✅ `graph_controller.cpp` has been removed and will not return.
- ✅ We will finish the ghost-edge workflow and, longer term, follow the seven-phase plan to remove every `qgraphicsitem_cast`.

All work must keep the application runnable; after every milestone we build/run before moving on.

## Risks & Mitigations
- **Risk:** Moving deletion logic into items could leave dangling references.  
  **Mitigation:** Implement typed collections first (Phases 1-3) so nodes/edges have self-contained ownership before flipping the Delete behavior.
- **Risk:** Incremental refactors might break startup or XML loading.  
  **Mitigation:** After each phase, build and launch the app, create/load a sample graph, and only proceed once everything works.
- **Risk:** Large refactors without version control discipline could lose work.  
  **Mitigation:** Commit and push to remote after every validated step.

## Incremental Task Roadmap (maintain a running app)
Each task ends with a build/run verification, then a commit & push.

### 1. **Baseline checkpoint**
Build & run current main branch to confirm green state; commit/push any prior cleanups.

**Git Branch:** `main` (direct commit)
**Files:** `concat.sh`, `plan.md`, `plan_response.md`, remove `javascript_console.*`, `graph_controller.cpp`

**Git Commands:**
```bash
git add concat.sh plan.md plan_response.md
git add -u  # Stage deletions
git commit -m "chore: Remove obsolete files and update documentation"
git push origin main
```

---

### 2. **Phase 1: Socket parent linkage**
Introduce explicit `Node* m_parentNode` so sockets stop casting back to their owner. Verify runtime.

**Git Branch:** `refactor/zero-cast-phase1-socket-parent`
**Files:** `socket.h`, `socket.cpp`, `node.cpp`
**Eliminates:** 1 qgraphicsitem_cast violation (socket.cpp:42)

**Changes:**
- Add `Node* m_parentNode` member to Socket
- Initialize in constructor
- Remove `qgraphicsitem_cast<Node*>(parentItem())`

**Git Commands:**
```bash
git checkout -b refactor/zero-cast-phase1-socket-parent
# ... make changes ...
git add socket.h socket.cpp node.cpp
git commit -m "refactor: Phase 1 - Add typed Socket parent pointer"
git push origin refactor/zero-cast-phase1-socket-parent
git checkout main
git merge refactor/zero-cast-phase1-socket-parent --no-ff
git push origin main
```

---

### 3. **Phase 2: Node socket collections**
Store typed input/output socket lists; update accessors; ensure scene interactions still work.

**Git Branch:** `refactor/zero-cast-phase2-node-collections`
**Files:** `node.h`, `node.cpp`
**Eliminates:** 2 qgraphicsitem_cast violations (node.cpp:188, 426)

**Changes:**
- Add `QList<Socket*> m_inputSockets` and `m_outputSockets`
- Add typed accessors: `inputSockets()`, `outputSockets()`
- Remove childItems() iteration with casts

**Git Commands:**
```bash
git checkout main
git checkout -b refactor/zero-cast-phase2-node-collections
# ... make changes ...
git add node.h node.cpp
git commit -m "refactor: Phase 2 - Add typed Socket collections to Node"
git push origin refactor/zero-cast-phase2-node-collections
git checkout main
git merge refactor/zero-cast-phase2-node-collections --no-ff
git push origin main
```

---

### 4. **Phase 3: Scene typed access**
Expose typed node/edge collections and replace remaining casts that walk `childItems()`; verify XML save/load.

**Git Branch:** `refactor/zero-cast-phase3-scene-collections`
**Files:** `scene.h`, `scene.cpp`, `window.cpp`
**Eliminates:** 4 qgraphicsitem_cast violations (scene.cpp:538, 641; window.cpp:895, 897)

**Changes:**
- Add `QList<Node*> m_nodes` and `QList<Edge*> m_edges` to Scene
- Add typed methods: `nodes()`, `edges()`, `addNode()`, `removeNode()`
- Update window.cpp to use typed collections

**Git Commands:**
```bash
git checkout main
git checkout -b refactor/zero-cast-phase3-scene-collections
# ... make changes ...
git add scene.h scene.cpp window.cpp
git commit -m "refactor: Phase 3 - Add typed Node/Edge collections to Scene"
git push origin refactor/zero-cast-phase3-scene-collections
git checkout main
git merge refactor/zero-cast-phase3-scene-collections --no-ff
git push origin main
```

---

### 5. **Phase 4: Delete-key overhaul** 🔥 CRITICAL
Add keyPress overrides to nodes/edges and delete the scene-level loop; regression-test deletion of single & multi-selection.

**Git Branch:** `refactor/zero-cast-phase4-delete-key`
**Files:** `node.h`, `node.cpp`, `edge.h`, `edge.cpp`, `scene.cpp`
**Eliminates:** 3 qgraphicsitem_cast violations (scene.cpp:454-458)

**Changes:**
- Add `keyPressEvent()` override to Node and Edge
- Each handles its own Delete key → `deleteLater()`
- REMOVE Scene::keyPressEvent deletion logic (lines 435-480)

**Git Commands:**
```bash
git checkout main
git checkout -b refactor/zero-cast-phase4-delete-key
# ... make changes ...
git add node.h node.cpp edge.h edge.cpp scene.cpp
git commit -m "refactor: Phase 4 - Move Delete key to item keyPressEvent"
git push origin refactor/zero-cast-phase4-delete-key
git checkout main
git merge refactor/zero-cast-phase4-delete-key --no-ff
git push origin main
```

---

### 6. **Phase 5: Ghost-edge polish**
Implement left-click drag completion (or harmonise button behavior) and confirm both creation paths work.

**Git Branch:** `refactor/zero-cast-phase5-ghost-edge`
**Files:** `socket.cpp`, `scene.cpp`
**Eliminates:** Reduces scene.cpp ghost edge casts via typed accessors

**Changes:**
- Complete TODO in socket.cpp:183 (left-click press starts ghost edge)
- Complete TODO in socket.cpp:204 (left-click release completes connection)
- Use typed Node::socketAtPoint() in scene.cpp where possible

**Git Commands:**
```bash
git checkout main
git checkout -b refactor/zero-cast-phase5-ghost-edge
# ... make changes ...
git add socket.cpp scene.cpp
git commit -m "refactor: Phase 5 - Complete ghost edge left-click drag"
git push origin refactor/zero-cast-phase5-ghost-edge
git checkout main
git merge refactor/zero-cast-phase5-ghost-edge --no-ff
git push origin main
```

---

### 7. **Phase 6+: Remaining cast removal**
Work through window/factory cleanup and any residual casts until zero remain, shipping after each safe subset.

**Phase 6 - Window cleanup:**

**Git Branch:** `refactor/zero-cast-phase6-window`
**Files:** `window.cpp`
**Eliminates:** Any remaining window.cpp casts

```bash
git checkout main
git checkout -b refactor/zero-cast-phase6-window
# ... make changes ...
git add window.cpp
git commit -m "refactor: Phase 6 - Remove remaining window.cpp casts"
git push origin refactor/zero-cast-phase6-window
git checkout main
git merge refactor/zero-cast-phase6-window --no-ff
git push origin main
```

**Phase 7 - Factory cleanup:**

**Git Branch:** `refactor/zero-cast-phase7-factory`
**Files:** `graph_factory.cpp`
**Eliminates:** 1 qgraphicsitem_cast violation (graph_factory.cpp:708)

```bash
git checkout main
git checkout -b refactor/zero-cast-phase7-factory
# ... make changes ...
git add graph_factory.cpp
git commit -m "refactor: Phase 7 - Remove graph_factory.cpp casts

🎉 MILESTONE: ZERO qgraphicsitem_cast in entire codebase"
git push origin refactor/zero-cast-phase7-factory
git checkout main
git merge refactor/zero-cast-phase7-factory --no-ff
git push origin main
```

**Final Verification:**
```bash
# Verify zero casts
grep -rn "qgraphicsitem_cast" --include="*.cpp" --include="*.h" . | grep -v "build_" | grep -v ".git"

# Tag milestone
git tag -a v1.0-zero-cast -m "Milestone: Zero qgraphicsitem_cast architecture"
git push origin v1.0-zero-cast
```

## Operational Rules
- Always build/run (or execute the relevant test suite) before and after every change set.
- Never leave the app in a broken state at the end of a task.
- Commit and push to the remote after every verified phase so no progress is lost.
Session saved: work committed to branch fix/shutdown-safety (07bc34f) for safe restart.
