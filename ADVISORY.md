# NodeGraph — Code Advisory Log

**Purpose of this document.** This is the running written record of the code-review conversation between the project owner and the reviewing assistant. Per the owner's instruction, everything stated in chat is also stated here, so this file and the conversation stay in sync as we go. New entries are appended at the bottom as dated sections; nothing is rewritten after the fact (corrections get their own dated notes).

**How to read entries.** Findings are tagged by severity:

- **[CRITICAL]** — crash, use-after-free, data loss, or a core feature broken.
- **[MAJOR]** — logic bug, broken invariant, incorrect Qt usage, undo/redo correctness.
- **[MODERATE]** — design smell, dead code, performance, error-handling gaps.
- **[NOTE]** — worth knowing; includes things that are genuinely *good*.

Every finding cites `file:line`. Line numbers refer to the working tree as of the entry date. Findings marked **(verified)** were re-confirmed by direct reading of the cited lines during the review; the rest come from the full-tree review pass and are cited precisely enough to check quickly.

---

## Entry 001 — 2026-07-22 — Initial full-codebase review

### 1. Scope and method

Reviewed the complete C++ source of the project (~40 files, ~6,500 lines): the QGraphicsItem core (`node`, `edge`, `socket`, `ghost_edge`), the `Scene` registry layer, the `Graph` facade with its type-erased `ScriptEngine` (QJSEngine + optional Duktape backends), the XML-first `GraphFactory` (libxml2), XML-snapshot undo commands, the observer/autosave layer, and the full UI (`window`, `view`, palette, `main`). Also read `CMakeLists.txt`, `README.md`, and the JS demos in `scripts/`.

Method: five parallel deep-review passes (one per subsystem), each required to cite code for every claim, followed by direct verification of the highest-impact findings. No automated tests exist in this project (all disabled in CMake), so every behavior claim below is from code reading, not execution.

### 2. Architecture as found

The short version, because several findings only make sense against this map:

- `Node` / `Edge` / `Socket` / `GhostEdge` are `QGraphicsItem`s — **not** `QObject`s. No signals/slots at item level; change notification goes through a raw function pointer (`Node::m_changeCallback`) and a `void* m_observer`.
- `Scene : QGraphicsScene, GraphSubject` owns two registries, `QHash<QUuid, Node*> m_nodes` and `QHash<QUuid, Edge*> m_edges`, which must be kept in sync with QGraphicsScene's own item ownership. Sync is maintained by destructor callbacks (`Node::~Node → Scene::notifyNodeDestroyed`, same for edges) plus a **process-wide static flag** `Scene::s_clearingGraph` that changes `Edge::~Edge` behavior during `clear()`.
- `GraphFactory` creates objects by serializing XML templates → `xmlParseDoc` → `Node::read` / `Edge::read` → add to scene. It also owns the file load path (`loadFromXmlFile`) with a two-phase all-or-nothing design.
- `Graph : QObject` is the scriptable facade (Q_INVOKABLE API). It holds a type-erased `ScriptEngine` (Sean Parent runtime-concept idiom — clean design) and registers itself as the JS global `graph`.
- `ScriptedNode : Node` stores JS source + a JSON payload, compiles via the shared engine, and runs on the UI thread.
- Undo is XML-snapshot `QUndoCommand`s (`CreateNodeCommand`, `ConnectEdgeCommand`, `DeleteSelectionCommand`, `MoveNodesCommand`) pushed only from `Window`.
- `XmlAutosaveObserver` debounces scene notifications into `autosave.xml` writes.
- **Three mutation paths exist and they are not unified**: (a) UI → `QUndoStack` → commands → factory/scene; (b) scripts → `Graph` facade → factory/scene directly (no undo); (c) file load → factory directly. Much of the fragility below traces back to this split.

### 3. Verified critical defects, ranked

These are the ones to fix first. Each was confirmed by reading the cited code.

**3.1 [CRITICAL] (verified) — File load rejects valid pass-through graphs.**
`graph_factory.cpp:519-520` builds duplicate-detection keys as `"%1:%2".arg(nodeId).arg(socketIndex)` with **no direction component**, and both the output side (:523) and input side (:541) are checked against the *same* `socketUsage` hash. Any node with an incoming edge on input *i* and an outgoing edge on output *i* — i.e., every `SOURCE → TRANSFORM → SINK` chain, the most basic pipeline — produces the same key twice and the load aborts with `MALFORMED FILE REJECTED` (:527, :545). Save a pass-through graph, reload it: fails. Fix: prefix the keys with direction (`"out:%1:%2"` / `"in:%1:%2"`).

**3.2 [CRITICAL] (verified) — Clean application exit overwrites `autosave.xml` with an empty graph.**
Chain: `Window::closeEvent` → `Scene::prepareForShutdown()` → `clearGraph()` → `Scene::clear()` → `notifyGraphCleared()` (`scene.cpp:345`) → `XmlAutosaveObserver::onGraphCleared` → `scheduleAutosave()` sets `m_pendingChanges = true` (`xml_autosave_observer.cpp:114-118`). The 1200 ms timer never fires before process exit, then `~Window` deletes the observer and `~XmlAutosaveObserver` flushes: `if (m_pendingChanges && m_enabled) saveNow();` (`xml_autosave_observer.cpp:41-42`) — serializing the just-cleared **empty** scene over the recovery file. The shutdown clear exists purely as a teardown-safety mechanic, yet it destroys the crash-recovery artifact on every normal exit. Notably, `newFile()` already knows to guard this (`window.cpp:1062-1064` disables autosave around the destructive op) — the shutdown path is missing the same guard, and `Scene::isShutdownInProgress()` exists but has **zero callers**. Fix: `m_autosaveObserver->setEnabled(false)` at the top of `closeEvent`, before `prepareForShutdown()`.

**3.3 [CRITICAL] (verified) — Incomplete manual weak-pointer system: edges keep dangling `Socket*` after node death.**
`Node::~Node` (`node.cpp:43-46`) calls `Edge::invalidateNode(this)`, which nulls only `m_fromNode` / `m_toNode` (`edge.cpp:78-87`). The edge's `m_fromSocket` / `m_toSocket` still point at the node's child sockets, which `~QGraphicsItem` destroys with the node. After that, `Edge::~Edge → detachSockets()` (`edge.cpp:224-239`) dereferences them (`m_fromSocket->getIndex()`), as does `Edge::updatePath()` (`edge.cpp:344`). Safety today rests entirely on two conventions: every deletion path deletes edges *before* nodes (true in `Scene::deleteNode`, `Scene::removeNode`, `DeleteSelectionCommand` — I checked each), or the static `s_clearingGraph` flag is set. The class that breaks the convention is base-class teardown itself: `Scene` has no destructor, so if shutdown ever skips `Window::closeEvent → prepareForShutdown`, `~QGraphicsScene` deletes items in arbitrary order with the flag unset → UAF at exit. The header comment "Manual weak pointers for safe destruction" (`edge.h:121`) advertises a guarantee the mechanism doesn't deliver for sockets. Fix: extend the invalidation to sockets — either `Node::~Node` also nulls the edge's socket pointers for its own sockets, or give `Socket` a destructor that detaches its connected edge (mirrors what `Edge::~Edge` already does for nodes).

**3.4 [CRITICAL] (verified) — Ghost-edge state survives `Scene::clear()` → UAF / double-delete.**
`Scene::clear()` (`scene.cpp:326-350`) calls `QGraphicsScene::clear()`, which deletes *all* items — including the scene-owned `GhostEdge` (added via `addItem`, `scene.cpp:393`) and the `Socket` that `m_ghostFromSocket` points to. Neither `clear()`, `clearGraph()` (:352), nor `prepareForShutdown()` (:362) resets `m_ghostEdge`, `m_ghostFromSocket`, or `m_ghostEdgeActive`. If a clear lands mid right-drag (File→New, a script's `graph.clearGraph()`, or shutdown), the next `mouseMoveEvent` reads `m_ghostFromSocket->scenePos()` (:414) — freed memory — and the next `cancelGhostEdge()` does `delete m_ghostEdge` (:547-550) on an already-deleted item — double-delete. Fix: call `cancelGhostEdge()` at the top of `Scene::clear()` (before `QGraphicsScene::clear()`).

**3.5 [CRITICAL] (verified) — Scripts can delete the nodes being iterated → UAF in script batch execution.**
`Window::runScriptsForNodes` (`window.cpp:1021-1033`) iterates a `QList<Node*>` of raw pointers while scripts run synchronously on the UI thread. Since the JS global `graph` is the `Graph` QObject itself (`graph.cpp:89`), any node script can call `graph.deleteNode(...)` or `graph.clearGraph()`, and `Scene::deleteNode` deletes **synchronously**. If node A's script deletes node B, the loop later dereferences B's freed pointer via `nodeHasScript(node)` → `node->getId()` (`window.cpp:958`). Reachable from the context menu ("Run Scripts for Selection" / "All Nodes"). Two adjacent variants of the same flaw: (a) a script deleting **its own** node leaves `ScriptedNode::evaluate` running on freed memory and `ScriptNodeApi::m_node` dangling (`scripted_node.cpp:139,184,194`); (b) a script calling `graph.setNodeScript(ownId, ...)` reassigns `m_compiledFunction` (`scripted_node.cpp:155-160`) while its `Model::call_` is mid-flight — destroying the type-erased callable from inside itself. Fix: (batch) snapshot UUIDs and re-resolve each iteration with `m_scene->getNode(id)`, skipping nulls; (self-deletion) guard `Graph::deleteNode`/`clearGraph` against reentry while a script is executing (a static execution-depth counter), or defer script-triggered deletions with `deleteLater`-style queuing; (self-rewrite) copy the function before the call: `const ScriptFunction fn = m_compiledFunction; fn.call(...)`.

**3.6 [CRITICAL] — `Edge::setResolvedSockets` fails silently; the factory then corrupts socket state.**
`edge.cpp:648-707`: on null socket / wrong role / already-connected it logs and returns early — return type `void`, so failure is undetectable. Caller `GraphFactory::connectSockets` (`graph_factory.cpp:256`) then **unconditionally** runs `fromSocket->setConnectedEdge(edge)` (:274-275): sockets end up pointing at an edge that never registered them. When that zombie edge is later destroyed, its `detachSockets` no-ops (pointers null) and the sockets keep a dangling `m_connectedEdge` — UAF on the next `isConnected()` check. Fix: return `bool` (like `resolveConnections` does, `edge.cpp:520`) and have the factory bail out; the two ~60-line near-identical validation blocks should become one helper.

**3.7 [CRITICAL] — Unbounded native recursion and no interruption: scripts can crash or freeze the host.**
`graph.executeNodeScript(...)` is Q_INVOKABLE (`graph.h:90`), so a node script can call it on itself: JS → C++ → `QJSValue::call` → JS → … Each level adds native frames and the JS engine's own recursion limit never trips because each entry is fresh. No depth guard exists anywhere. Same under Duktape. Additionally `while(1){}` in any script freezes the UI thread forever — `QJSEngine::setInterrupted()` (Qt ≥5.14) is never used, and `SyntheticWork` accepts unbounded `iterations`/`delayMs` (`synthetic_work.cpp:26,55`) with `QThread::msleep` on the UI thread. Fix: execution-depth counter in `ScriptedNode::evaluate` (bail out past ~8); watchdog timer driving `setInterrupted()`; clamp synthetic-work parameters.

**3.8 [CRITICAL, Duktape-only] — `getVariant` recurses without depth/cycle guard.**
`duktape_script_backend.cpp:108-118` enumerates own properties recursively; a script returning (or passing) a cyclic object (`var o={}; o.me=o;`) overflows the native stack. Only relevant when `ENABLE_DUKTAPE=ON`, but it's the kind of bug that will be forgotten until someone enables the backend. Fix: depth-limit parameter or a visited-pointer set.

### 4. Major issues (selected, all cited)

**4.1 [MAJOR] (verified) — `prepareGeometryChange()` after the mutation in `Node::calculateNodeSize`.** `node.cpp:522-537` mutates `m_width`/`m_height`, then calls `prepareGeometryChange()` at :540. Qt requires it *before* changing anything that affects `boundingRect()`; `setNodeSize` right above (:154-160) does it correctly, so this is inconsistency, not misunderstanding. Scene spatial index updated against already-changed geometry.

**4.2 [MAJOR] — `Node::paint` inks ~7 px outside `boundingRect()`.** Selection glow `rect.adjusted(-3,-3,3,3)` with pen width 8 (`node.cpp:75-77`) exceeds the declared `(0,0,w,h)` rect (:53-56) → repaint smearing when selected nodes move. Fix: inflate `boundingRect()` by the glow margin (Edge already does this via `kPickRadius`).

**4.3 [MAJOR] (verified) — `Edge::buildPath` treats scene coordinate (0,0) as invalid.** `edge.cpp:355`: `start.isNull() || end.isNull()` — `QPointF::isNull()` is true for exact `(0,0)`, a legal scene position. Any edge whose endpoint maps to the origin silently vanishes. Fix: drop the `isNull()` checks, keep the `qIsFinite` ones.

**4.4 [MAJOR] — Load reports success over corrupt graphs.** Three parts: (a) Phase-1 validation never checks `fromNode`/`toNode` reference nodes present in the file, and Phase 3c treats resolution failure as non-fatal (`graph_factory.cpp:594-601`) — dead edges stay in the scene forever and are saved back out; (b) no duplicate-UUID validation, and `QHash::insert` **overwrites** (`scene.cpp:45,66`), so duplicate ids create untracked zombie items; (c) `resolveConnections` doesn't reject self-loops while runtime `connectSockets` does — inconsistent policy between file and interactive paths. The two-phase all-or-nothing load design is genuinely good; these holes undermine it.

**4.5 [MAJOR] — Facade/UI/undo incoherence.** All `Graph` facade mutations bypass the undo stack (`graph.cpp:117,145,299,327,476`); `Graph::loadFromFile` loads **without clearing first** (`graph.cpp:557-581` — only `Window::loadGraph` clears, so scripts merge files into the live graph); `Graph::clearGraph` doesn't clear the undo stack (stale XML-snapshot commands then resurrect dead nodes on undo); facade signals fire only on the facade path, so JS listeners never see UI-created items; `Scene::deleteNode`'s cascade deletes incident edges without emitting `Graph::edgeDeleted`. Three paths, three different histories. This is the single largest *architectural* problem in the codebase — the fix is routing all three through one command layer.

**4.6 [MAJOR] (verified) — Right-button release swallowed; socket stuck pressed; source socket stuck `Connecting`.** `Scene::mouseReleaseEvent` finishes the ghost edge and returns without calling `QGraphicsScene::mouseReleaseEvent` (`scene.cpp:613-618`) — the grabber socket never receives its release (`Socket::m_pressed` stuck, Qt's mouse-grabber state stale). Separately, `cancelGhostEdge` nulls `m_ghostFromSocket` without resetting its `Connecting` state (:398 vs :458/:552) on every cancel/invalid-drop path.

**4.7 [MAJOR] (verified) — Shortcut layer chaos.** (a) `Ctrl+1` is bound twice: Add Input (`window.cpp:150`) and Reset Zoom (`window.cpp:592`) — ambiguous, Reset Zoom unreachable; (b) five menu actions are created and added to menus but never `connect()`ed (`selectAll`/`deselect`/`delete`/`validate`/`statistics`, window.cpp:549-635) — clicking them does nothing; (c) Delete is registered three ways (two `QShortcut`s :163-173, an unconnected `deleteAction` :562, and a `keyPressEvent` branch :242-247); (d) `Window::keyPressEvent` re-implements Ctrl+S/O/1-3 and has already **diverged** from the QAction paths (hand-rolls `m_currentFile`/title updates instead of `setCurrentFile()`). Fix: one layer — connect the QActions, delete the QShortcuts and the keyPressEvent duplicates. Also note: Backspace with `WidgetWithChildrenShortcut` context will fire while the user types in any text widget — worth checking once scripted nodes embed editors.

**4.8 [MAJOR] (verified) — Uninitialized `m_autosaveObserver`.** `window.h:152` declares the raw pointer with no initializer and the ctor doesn't set it; first write is `adoptFactory()` (`window.cpp:88`). Read in `~Window` (:59), `loadGraph` (:349), `newFile` (:1062). Currently masked because `main()` adopts before `show()`. One-line fix: `= nullptr;` in the header. Same class of bug: `adoptFactory` is not idempotent (second call leaks `m_graph`/observer and double-attaches).

**4.9 [MAJOR] (verified) — `SyntheticWork::run` measures nothing.** `synthetic_work.cpp:73-85`: `timer.elapsed()` is evaluated at argument-passing time — *before* `runLoop`/`runHash`/`runDelay` execute — so `durationMs` is always ~0. The headline metric of the feature, and every demo script logs it. Fix: sample after the task returns.

**4.10 [MAJOR] — Global un-RAII'd batch mute with no end-of-batch flush.** `GraphSubject::s_batchDepth` (`graph_observer.cpp:11`) is process-global; every node/edge notification is muted while >0. A single unbalanced `beginBatch()` (early return in the six-exit load path, or a JS script that throws) permanently silences all notifications and autosave application-wide, with no diagnostic. And `endBatch()` emits no catch-up notification (`graph_observer.cpp:152-168`), so batched mutations never reach autosave — your own `PLAN.MD:59` already lists "fire autosave once per batch" as planned-but-unimplemented. Separately, `Graph::beginBatch/endBatch` (`graph.cpp:458-469`) is a **different, fake** mechanism — a flag nothing reads. Two parallel "batch" concepts, one real and dangerous, one inert.

**4.11 [MAJOR] — Observer container unsafe under reentrancy.** Notify loops range-for over the raw `QSet<GraphObserver*>` while invoking virtual handlers (`graph_observer.cpp:48-52` etc.); an observer detaching or dying inside a callback invalidates the iteration or dangles for the next notify. `GraphObserver`'s dtor doesn't self-detach — only `Window`'s explicit detach prevents a UAF today. Fix: detach in `~GraphObserver`, snapshot before iterating.

**4.12 [MAJOR] — Autosave writes UTF-8 through the locale codec; non-atomic write.** `xml_autosave_observer.cpp:158-160`: `QTextStream out(&file); out << xmlContent;` with no `setCodec` — Qt 5 defaults to the locale codec (ANSI on Windows) while the content declares `encoding="UTF-8"`. Non-ASCII script/name bytes get corrupted. Also the file is truncated in place (a crash mid-write destroys the recovery file — the exact failure autosave exists for); write `autosave.xml.tmp` + rename. And the `catch(...)` around `node->write` (:231-238) swallows everything then logs the partial write as success.

**4.13 [MAJOR] — libxml2 parse hardening.** `xmlParseFile` with default flags (`graph_factory.cpp:311`): internal entity expansion enabled (billion-laughs DoS on untrusted files), no `xmlGetLastError()` detail on failure, and on Windows the path goes to ANSI `fopen` — non-ASCII file paths fail. Fix: `xmlReadFile(path, nullptr, XML_PARSE_NONET)` + error-detail logging. Also `xmlInitParser` is never called (fine single-threaded, required if parsing ever leaves the GUI thread).

**4.14 [MAJOR] — Static shared engine + `Graph` dtor reset = latent UAF.** `Graph` installs its engine into `ScriptedNode::s_engine` (static) at `graph.cpp:70` and resets it in the dtor (:77). Existing `ScriptedNode`s keep compiled functions co-owning an engine heap whose global `graph` wraps the dead `Graph` (CppOwnership wrappers are **not** nulled on QObject death). No current call path triggers it (guarded compile at `scripted_node.cpp:288` helps), but a second `Graph` instance or a shutdown-time evaluation turns it into a UAF. The single-Graph invariant is nowhere enforced.

**4.15 [MAJOR] — Palette button clicks do nothing.** `nodeCreationRequested` is emitted (`node_palette_widget.cpp:241`) but has **zero connections** anywhere in the repo — the tooltip promises "Drag to create or double-click", only drag works. `PaletteButton` class is entirely dead code. Also the drag mime format is a `|`-joined string that breaks on any `|` in a name.

**4.16 [MAJOR] — `sceneChanged` connected twice → status bar updates 2× per change.** `window.cpp:125` (via `onSceneChanged`) and `window.cpp:712` (direct). Each update runs `getGraphStats()` plus a flushed `qDebug` line. Combined with per-message `fflush` + `OutputDebugStringW` in the log handler (`main.cpp:71-75`) and qDebug calls in virtually every hot path (paint-adjacent, hover, drag, per-notify, per-node-create), logging is a real per-frame cost — and the "temporary diagnostics" instance counters (`node.h:45`) have been permanent since they were added.

**4.17 [MAJOR] — `main()` teardown order is convention-based.** `Window window;` (`main.cpp:166`) is constructed before `GraphFactory factory` (:178), so the factory dies **first** at scope exit — leaving `Window::m_factory`, `Scene::m_graphFactory`, and `Graph::m_factory` dangling through `~Window`/`~Graph`. Safe today only because no destructor dereferences them. `xmlFreeDoc(xmlDoc)` (:258) likewise runs before factory/window destruction. Fix: declare `factory` before `window`, or bundle doc+factory into a small RAII context. Adjacent: the "File Not Found" QMessageBox path (:243-250) is dead code — `return -1` at :212 fires first; decide which behavior you want.

**4.18 [MAJOR] — Unbounded zoom + unbounded grid loop.** `View::wheelEvent` multiplies scale by 1.15 per tick with no clamp (`view.cpp:110-119`); `drawBackground` then loops over the exposed rect at 20 px spacing (:228-259) — at extreme zoom-out that's tens of thousands of `drawLine` calls per repaint, and `FullViewportUpdate` (:31) forces full repaints. Clamp the cumulative scale.

**4.19 [MAJOR] — Undo command edge cases.** (a) A failed re-snapshot in `undo()` overwrites the last good XML snapshot (`undo_commands.cpp:149`, :194) → permanent redo loss; only assign when serialization is non-empty. (b) `DeleteSelectionCommand::undo` ignores restore failures (:254-259) — a failed node restore cascades into edge deletions with log-only warnings: partial undo silently loses data. (c) `CreateNodeCommand::redo` doesn't `setObsolete` on restore failure. (Memory verdict on the snapshot machinery itself, for the record: **clean** — element ownership through `write()` → `xmlDocSetRootElement` → `xmlFreeDoc` verified on all paths, no leaks, no double-free.)

**4.20 [MAJOR] — Template injection by substring sniffing.** `node_templates.cpp:205,214`: `if (!result.contains("id="))` / `contains("x=")` scan the whole template **including the embedded script**. Any registered template whose JS contains `x=` (e.g. `var x=1;`) or `id=` skips id/position injection — the node then keeps a random ctor UUID (mismatch with the id the caller was given) or lands at (100,100) instead of the drop point. Built-ins happen to be clean today; `registerTemplate` is public API, so it's a latent trap. Fix: real placeholders (`{{ID}}`, `{{X}}`) or an XML writer.

### 5. Systemic themes (the "why" behind the list)

Stepping back, five root causes generate most of the findings above. Fix the theme and you prevent the next ten findings, not just the one:

1. **Ownership by convention.** The codebase has no destructor-level enforcement of its lifetime rules — it has *call-site discipline* (edges before nodes), a static flag (`s_clearingGraph`), raw `void*` observers, and comments saying "non-owning". Every one of findings 3.2–3.6, 4.8, 4.14, 4.17 is a place where the convention holds *today by accident of call order*. The single highest-leverage change: make `Socket`/`Edge`/`Node` destructors symmetric (each item detaches what it points at), so order stops mattering.

2. **Three mutation paths, one graph.** UI/undo, script facade, and file load each mutate the scene differently (§4.5). Until all mutations funnel through one layer (ideally the command layer, with the facade emitting the same signals), every feature that listens to changes (autosave, status bar, JS listeners) will keep missing one of the paths.

3. **Fake/stub API surface presented as real.** `Graph::beginBatch/endBatch` (flag only), `Graph::toXml()` (returns a constant `"<graph></graph>"` — a Q_INVOKABLE that silently returns wrong data), `GraphFactory::createEdge`/`createXmlNode` (no callers), `Scene::removeNode/removeEdge` (dead + inconsistent with deleteNode), `Scene::snapPoint` (dead, and disagrees with View's grid: 40 vs 20 px), the scratch `m_xmlDocument` edge nodes with wrong random UUIDs (`graph_factory.cpp:266-271`), `node_templates` JS/file loaders returning placeholders. Each one misleads the next reader — worst of all the JS consumer, who has no compiler to catch it. Prune or implement; nothing in between.

4. **Comments contradict code.** The Delete-key story is told three different ways (`node.h:61` "centralized in Scene", `edge.cpp:269` "centralized in Window", `scene.cpp:322` "each item handles its own" — reality is Window shortcuts + three no-op overrides). `node.h:33` still claims "value semantics" for heap-owned scene items. `scripted_node.cpp:174` says context is unused; it's forwarded at :185. Five stale comments around key handling alone. When comments lie, reviewers (and future you) stop trusting the true ones.

5. **Debug scaffolding shipped in hot paths.** Unguarded `qDebug` in create/destroy of every node/edge, per-hover logs, per-notify logs, per-move logs, a 10-line banner per palette drop, plus a log handler that `fflush`es to disk per message. On this codebase's own terms (it profiles node creation at `graph_factory.cpp:147-153`), that's self-inflicted noise. Route it through a category logger (`QLoggingCategory`) and guard the "temporary" counters with `#ifdef QT_DEBUG`.

### 6. What's genuinely good (calibration matters)

- The **two-phase all-or-nothing load** in `GraphFactory` (validate everything, then mutate) is the right architecture — its holes (§3.1, §4.4) are bugs *in* it, not a bad design.
- The **XML-snapshot undo machinery is memory-clean** — element ownership through `write()`/`xmlDocSetRootElement`/`xmlFreeDoc` verified on all paths; UUIDs survive undo/redo round-trips, so cross-command references stay valid; construct-time snapshot in `DeleteSelectionCommand` is semantically correct.
- The **type-erased `ScriptEngine` seam** (`script_engine.h`) is a faithful Sean Parent runtime-concept implementation with vocabulary types only — backends plug in structurally, nothing engine-specific leaks. The QJS backend's ownership handling (`CppOwnership` on stack objects before `newQObject`) is correct. The Duktape dispatcher deliberately avoids `duk_error` longjmp over C++ locals — whoever wrote that understood the ABI hazard.
- **Script/payload XML escaping is delegated to libxml2 correctly** (`xmlNodeSetContent`/`xmlNodeGetContent`), CDATA templates round-trip, and all `xmlChar*` frees in `Edge::read` are balanced on every path.
- `Edge`'s cached `static const QPen` pattern, `Node`'s `ItemPositionHasChanged` + cumulative 5 px threshold with per-node `m_lastPos`, and `MoveNodesCommand::m_firstRedo` skip are all correct, thoughtful patterns.
- The `ScopedClearing` RAII guard and the delete-then-notify *before* delete ordering in `Scene::deleteNode` (observers never see dangling pointers) are right.
- The qgraphicsitem_cast elimination (your stated current focus) is real: typed registries, `m_parentNode` back-pointers, and `dynamic_cast` in the one remaining UI spot.

### 7. Recommended fix order

Grouped so each pass is independently shippable:

**Pass 1 — data-loss & crash fixes (small diffs, high value):**
1. §3.1 direction-prefixed socket keys (load rejects valid files).
2. §3.2 autosave disable before shutdown clear (recovery file wiped on exit).
3. §3.4 `cancelGhostEdge()` in `Scene::clear()` (UAF/double-delete).
4. §4.8 `m_autosaveObserver = nullptr` (+ idempotency guard on `adoptFactory`).
5. §4.9 measure `durationMs` after the work (headline metric is 0).
6. §4.3 drop `isNull()` checks in `buildPath` (edges vanish at origin).
7. §4.1 move `prepareGeometryChange()` to the top of `calculateNodeSize`.

**Pass 2 — lifetime hardening:** §3.3 symmetric socket invalidation (plus `~Socket` detach), §3.6 `setResolvedSockets` → `bool`, §3.5 script-execution reentry guards + UUID-snapshot iteration in `runScriptsForNodes`, §4.17 main() ordering, §4.11 observer self-detach.

**Pass 3 — script-host safety:** §3.7 depth guard + `setInterrupted` watchdog + synthetic-work clamps; §3.8 if Duktape is on your roadmap.

**Pass 4 — coherence:** §4.5 route facade mutations through the command layer (or at minimum: facade load clears first, facade clear resets undo, facade emits on all paths); §4.10 RAII batch guard + end-of-batch flush; delete/merge the fake batch API and the other stubs (§5.3).

**Pass 5 — UX & hygiene:** §4.7 shortcuts, §4.15 palette click wiring, §4.18 zoom clamp, §4.16/§5.5 logging diet + dead-code pruning + comment reconciliation. §4.13 XML parser hardening can ride any pass.

### 8. Open questions for you (these change what "correct" means)

1. **Is wiping `autosave.xml` on clean exit ever the intent?** If yes, it should happen explicitly (delete the file on clean exit, keep it only after a crash) — not as a side effect of the shutdown clear. If no, §3.2 is the fix as stated.
2. **Should script-driven mutations be undoable?** That decides whether §4.5 means "facade pushes commands" (undoable scripts, bigger change) or "facade emits the same signals and clears undo on structural ops" (smaller change).
3. **Is Duktape a real roadmap item?** If yes, §3.8 and the stash leak (`duktape_script_backend.cpp:365-371`) move up; if not, consider deleting the backend to shrink the surface.
4. **Is a second `Scene`/`Graph` instance ever planned** (multiple documents, tests)? If yes, the statics (`s_clearingGraph`, `s_batchDepth`, `ScriptedNode::s_engine`) all need redesign, not patching.
5. **What should left-click on a socket do?** `socket.cpp:184,205` still carry `TODO: start edge creation` from the abandoned left-drag design; right-drag is the live one. Decide and delete the other path.

### 9. Where we go next

This entry covers the full tree at review depth. Natural next steps, your call: (a) work through Pass 1 together — each fix is small and I can implement + verify them one at a time; (b) go deeper on any single finding into a full root-cause + patch design; (c) a second review pass focused on one subsystem you care about (e.g. the script-host boundary, which has the highest exploit density). Whatever we discuss gets appended here as Entry 002, 003, … with the same finding format.

---

## Entry 002 — 2026-07-22 — Status confirmation & document recap

Context: the owner asked whether the advisory was written, what it contains, and what their current usage rate is.

**File status (verified on disk).** `ADVISORY.md` exists at the project root: 30,584 bytes, 162 lines, written 2026-07-22 05:41 CDT. Entry 001 is complete and intact.

**Recap of Entry 001's content.**

- **§1–2:** Scope (full tree, ~40 files, ~6,500 lines; five parallel review passes + direct verification of the top findings) and an architecture map — QGraphicsItem core with no QObject/signals at item level, `Scene` UUID registries synced to Qt ownership by destructor callbacks plus the static `s_clearingGraph` flag, XML-first `GraphFactory`, the `Graph` facade with its type-erased `ScriptEngine`, XML-snapshot undo, and three un-unified mutation paths (UI/undo, script facade, file load).
- **§3 — 8 verified critical defects, ranked:** (3.1) load rejects valid pass-through graphs via direction-less socket keys; (3.2) clean exit wipes `autosave.xml`; (3.3) edges keep dangling `Socket*` after node death (incomplete manual weak pointers); (3.4) ghost-edge UAF/double-delete across `Scene::clear()`; (3.5) script-driven node deletion causes UAF in batch script execution (plus self-deletion and live-`setScript` variants); (3.6) `Edge::setResolvedSockets` fails silently and the factory corrupts socket state; (3.7) unbounded native recursion + no script interruption; (3.8) Duktape `getVariant` unguarded recursion (backend-conditional).
- **§4 — 20 major issues,** each cited, incl.: `prepareGeometryChange()` after mutation; node paint outside `boundingRect()`; edge vanishing at scene origin; load reporting success over corrupt graphs; facade/undo/UI incoherence; swallowed right-button release; shortcut chaos (double-bound Ctrl+1, five unconnected menu actions); uninitialized `m_autosaveObserver`; `SyntheticWork::run` measuring ~0 ms; global batch mute with no end-of-batch flush; observer container unsafe under reentrancy; autosave encoding corruption + non-atomic writes; libxml2 parse hardening; static script-engine lifetime; dead palette click path; double-connected `sceneChanged`; `main()` teardown order; unbounded zoom; undo snapshot edge cases; template injection by substring sniffing.
- **§5 — Five systemic themes:** ownership by convention; three mutation paths, one graph; fake/stub API surface presented as real; comments contradicting code; debug scaffolding shipped in hot paths.
- **§6 — What's genuinely good:** all-or-nothing load design; memory-clean undo snapshots; the Sean Parent script-engine seam; correct QJS ownership handling; correct libxml2 escaping; several correct hot-path patterns; real qgraphicsitem_cast elimination.
- **§7 — Recommended fix order in five passes:** Pass 1 data-loss/crash quick fixes → Pass 2 lifetime hardening → Pass 3 script-host safety → Pass 4 facade/undo coherence → Pass 5 UX & hygiene.
- **§8 — Five open questions** for the owner that change what "correct" means (autosave-on-exit intent, undoable scripts, Duktape roadmap, multi-instance plans, left-click socket behavior).
- **§9 — Next-step options:** work Pass 1 fixes one at a time, deep-dive any single finding, or a focused second pass on the script-host boundary.

**Usage rate.** I cannot read account quota from inside the session. The documented way: type `/usage` on the Kimi Code CLI command line, or log in to the Kimi Code Console to see remaining quota and rate-limit status. (Source: official docs, [Membership Benefits](https://www.kimi.com/code/docs/en/kimi-code/membership.html).)

**Awaiting the owner:** pick a next step from §9 — the recommended starting point is Pass 1 (seven small, independently shippable fixes).

---

## Entry 003 — 2026-07-22 — Testing strategy for the fix phases

Context: the owner asked "how do we test these items". This entry records the agreed approach. It assumes the plan's phases (A = quick data-loss fixes, B = lifetime hardening, C = script-host safety + facade coherence).

### 1. The three verification layers

**Layer 1 — Compile gates (already in use).** After each phase: `cmake --build build_Debug --config Debug --parallel` must complete clean. This catches API mismatches, missing definitions, header cycles. It proves the code builds — nothing more. Phase A and B gates PASSED this way.

**Layer 2 — Scripted smoke tests via the app's own `--script` hook.** The app executes a JavaScript file 50 ms after the window shows (`Window::showEvent`), with the `Graph` facade exposed as the JS global `graph`, and every `qDebug`/`console.log` line lands in `logs/NodeGraph_<timestamp>.log` (the message handler in `main.cpp` writes to disk). That is a complete automation harness without resurrecting any test framework:

- Write `scripts/smoke_fixes.js` that drives the facade and logs one `TEST <name> PASS` / `TEST <name> FAIL <detail>` line per check.
- Run: `build_Debug/Debug/NodeGraph.exe --script scripts/smoke_fixes.js` (a window briefly opens — expected; the app can then be closed manually or killed after ~15 s; results are in the newest `logs/NodeGraph_*.log`).
- Grep the log for the markers.

**Layer 3 — Manual GUI checklist** for the items a script physically cannot do (mouse gestures, window close). Kept short on purpose.

### 2. Per-item test matrix

| Fix | What it proves | Layer |
|---|---|---|
| A1 pass-through load | Create SOURCE→TRANSFORM→SINK, save, clear, load back: 3 nodes + 2 edges survive (was: load rejected) | 2 (scripted) |
| A2 autosave kept on exit | Create node, wait >1.2 s for autosave, close app cleanly: `autosave.xml` still contains the node (was: overwritten empty) | 2+3 (script builds, close window, inspect file) |
| A3 ghost-edge UAF on clear | Right-drag a connection, File→New mid-drag, keep moving the mouse: no crash | 3 (manual only — needs real mouse) |
| A4 init/idempotency | Latent UB — covered by build + normal run; no dedicated test | 1 |
| A5 durationMs | `graph.runSyntheticWork({task:"loop",iterations:200000})` returns `durationMs > 0` (was: ~0) | 2 (scripted) |
| A6 edge at origin | Node at (0,0), connect out: edge exists in `graph.getAllEdges()` (was: silently blanked) | 2 (scripted) |
| A7 prepareGeometryChange | Spatial-index correctness — build + visual inspection while dragging | 1+3 |
| B1 socket weak pointers | Create graph, delete nodes/edges repeatedly, close app: no crash on teardown | 2+3 |
| B2 resolve failure reporting | Connect the same output socket twice via `graph.connectNodes`: second call returns `""` and the first edge still works (was: zombie wiring) | 2 (scripted) |
| B3 script self-deletion | Node script calling `graph.deleteNode(ownId)` is refused with an error; app and remaining nodes alive (was: UAF) | 2 (scripted) |
| B4 teardown order | Clean exit after using the app: no exit crash | 3 |
| B5 observer self-detach | Covered by B1's create/delete cycles + clean exit | 2+3 |
| C1 recursion cap | Node script calling `graph.executeNodeScript(ownId)` stops at depth 8 with a warning (was: stack overflow) | 2 (scripted) |
| C2 watchdog | Node script with `while(1){}` is interrupted after ~5 s; app responsive, error logged (was: permanent freeze) | 2 (scripted, slow by design) |
| C3 work clamps | `runSyntheticWork({task:"delay",delayMs:999999})` clamps to 5000 ms (was: ~16 min freeze) | 2 (scripted) |
| C5 batch flush | `beginBatch` → create node → `endBatch` → autosave fires once (log shows the catch-up) — depends on the pending facade-batch decision below | 2 (scripted) |
| C6 facade load clears | Load a file into a non-empty graph via script: node count equals file's count, not the sum (was: silent merge) | 2 (scripted) |
| C7 toXml honesty | `graph.toXml()` returns real XML or a clear "not implemented" error, never a constant fake | 2 (scripted) |

### 3. What we deliberately do NOT test

- No unit-test framework resurrection (project disabled it by choice; out of scope).
- Duktape-backend paths (C4) — `ENABLE_DUKTAPE=OFF` here; those changes are verified by reading, and optionally by a one-off `-DENABLE_DUKTAPE=ON` configure if the fetch works.
- Multi-instance statics (`s_clearingGraph`, `s_batchDepth`, `s_engine`) — documented limitation, untestable without a second Scene.

### 4. Open decision blocking the C5 test (carried from the batch-mode discussion)

The facade's `Graph::beginBatch/endBatch` is still the fake `m_batchMode` flag (HEAD and tree both confirm the earlier AI built the real `GraphSubject` batching but never wired the facade pair to it — history checked via `git log`/`git show 8f3d41e`). The tree currently does **not link** because `graph.h:178` declares `isBatchMode()` without a definition (its definition was in the edit the owner paused on). Options presented to the owner: (1) forward the facade pair to `GraphSubject` and define `isBatchMode()` via `isInBatch()` — recommended, completes the earlier AI's direction; (2) restore the inline fake stub so the tree builds, leaving the facade pair as no-ops; (3) make them warn-and-no-op. C5's scripted test only makes sense under option 1. Awaiting the owner's pick.

---

## Entry 004 — 2026-07-22 — Remediation complete: Phases A–C, verification evidence, open work

### 1. What was done

All items from the approved fix plan (Passes 1–3 of Entry 001 §7) are implemented. Three incremental Debug build gates PASSED (one mid-gate error — MSVC C3493 on a lambda-captured `constexpr` — was found by the gate and fixed by moving the constant to namespace scope).

**Phase A — quick data-loss/crash fixes:**

| ID | Finding (Entry 001) | Fix | Files |
|---|---|---|---|
| A1 | §3.1 load rejects pass-through graphs | Direction-prefixed duplicate-socket keys (`out:`/`in:`) | `graph_factory.cpp` |
| A2 | §3.2 clean exit wipes autosave.xml | Autosave disabled in `closeEvent` + shutdown guard in `scheduleAutosave` | `window.cpp`, `xml_autosave_observer.cpp` |
| A3 | §3.4 ghost-edge UAF/double-delete on clear | `cancelGhostEdge()` first in `Scene::clear()` + source-socket state restore | `scene.cpp` |
| A4 | §4.8 uninitialized observer / non-idempotent adopt | `= nullptr` in-class init + idempotency guard | `window.h`, `window.cpp` |
| A5 | §4.9 durationMs always ~0 | Helpers sample the timer after the work | `synthetic_work.cpp` |
| A6 | §4.3 edge vanishes at (0,0) | Dropped `isNull()` disjuncts, kept `qIsFinite` | `edge.cpp` |
| A7 | §4.1 prepareGeometryChange after mutation | Moved to top of `calculateNodeSize` | `node.cpp` |

**Phase B — lifetime hardening:**

| ID | Finding | Fix | Files |
|---|---|---|---|
| B1 | §3.3 dangling `Socket*` in edges | `Socket::~Socket` detaches via new `Edge::invalidateSocket` — the weak-pointer system is now symmetric | `socket.h/.cpp`, `edge.h/.cpp` |
| B2 | §3.6 silent resolve failure → zombie wiring | `setResolvedSockets` returns `bool`; factory bails and deletes the rejected edge | `edge.h/.cpp`, `graph_factory.cpp` |
| B3 | §3.5 script deletes nodes under running code | Static execution-depth counter; `Graph::{deleteNode,clearGraph,deleteSelection,loadFromFile}` refuse mid-script; `evaluate()` copies the `ScriptFunction` before calling; batch script runner snapshots UUIDs and re-resolves | `scripted_node.h/.cpp`, `graph.cpp`, `window.cpp` |
| B4 | §4.17 main() teardown order | `QScopedPointer` factory/window + explicit window→factory→doc teardown; dead File-Not-Found block removed | `main.cpp` |
| B5 | §4.11 observer container unsafe | Observer self-detach on destruction (with subject tracking), subject unregisters on death, all notify loops iterate copies | `graph_observer.h/.cpp` |

**Phase C — script-host safety + facade coherence:**

| ID | Finding | Fix | Files |
|---|---|---|---|
| C1 | §3.7 native recursion | Depth cap 8 in `evaluate()` (landed with B3) | `scripted_node.cpp` |
| C2 | §3.7 no script interruption | `ScriptEngine::interrupt()` seam; QJS backend via `setInterrupted` (reset in evaluate/call/**compile** — see §3 below); 5 s watchdog thread on the outermost evaluate | `script_engine.h`, `qjs_script_backend.h/.cpp`, `scripted_node.cpp` |
| C3 | uncapped C++ work | `iterations` ≤ 10 M, `delayMs` ≤ 5000, with warnings | `synthetic_work.cpp` |
| C4 | §3.8 Duktape recursion + stash leak | `getVariant` depth guard (32); `Compiled::StashSlot` shared ownership deletes stash entries | `duktape_script_backend.h/.cpp` |
| C5 | §4.10 fake batch / no flush | `GraphSubject::BatchGuard` (RAII) replaces the factory's 5 manual `endBatch()` calls; `onBatchEnded()` flush to all subjects' observers; autosave catch-up override; facade `beginBatch/endBatch/isBatchMode` forward to the real mechanism (`m_batchMode` deleted) | `graph_observer.h/.cpp`, `graph_factory.cpp`, `graph.h/.cpp`, `xml_autosave_observer.h/.cpp` |
| C6 | §4.5 facade incoherence | `Graph::loadFromFile` clears first (replace, not merge); `graphCleared`/`graphLoaded` clear the undo stack; verified `Scene::deleteNode` cascade already notifies per edge | `graph.cpp`, `window.cpp` |
| C7 | fake `toXml()` | Real scene serialization to string via libxml2 | `graph.cpp` |

### 2. Verification evidence

- **Build gates:** Phase A, B, C gates all PASSED (incremental Debug, MSBuild, clean).
- **Scripted smoke (`scripts/smoke_fixes.js`, run via `NodeGraph.exe --script`): 12/12 PASS** — setup.chain, A1.save, setup.cleared, A1.load.passthrough, C6.load.replaces, B2.dup.refused, A5.durationMs, C3.delay.clamped, B3.selfdelete.refused, C1.recursion.capped, C5.batch.real, C7.toXml.real. (First run was 10/12 due to a bug in the test script itself — the "stray" node was created before saving and thus legitimately inside the fixture file; corrected, not a code defect.)
- **Watchdog (`scripts/smoke_watchdog.js`):** infinite `while(1){}` node script interrupted at ~5.5 s; app stayed alive. Note: the interrupt also unwinds the calling outer script — inherent `QJSEngine::setInterrupted` behavior, hence the separate script.
- **Found by testing, fixed on the spot:** stale interrupt flag could fail a later `compile()` (`QJsBackend::compile` now resets it too). **Caveat: this final one-liner was not rebuild-verified before commit (owner redirected to commit); it is a single call to an API used twice already in the same file.**
- **Manual checklist still open (Layer 3):** A2 clean-exit autosave inspection; A3 right-drag + File→New mid-drag; B1/B4 repeated delete cycles + clean exit; A7 visual drag/hit-test sanity.

### 3. What "a testing application" might look like (design, on request)

- **Tier 1 — done:** the `--script` smoke harness above. Zero new infrastructure; tests the real app end-to-end through the actual JS surface; results grepped from `logs/`.
- **Tier 2 — proposed, not built:** a `NodeGraphTests` console app. The skeleton already exists commented-out in `CMakeLists.txt` (~lines 303-363); `Qt5::Test` is already linked. Qt Test with `QTEST_MAIN` (picks `QApplication` automatically — required for QGraphicsItems). Test classes: `FactoryLoadTests` (A1 round-trips, duplicate-UUID rejection), `FacadeTests` (B2, C6, C7), `ScriptSafetyTests` (B3, C1, C2-as-QSKIP), `ObserverBatchTests` (C5 flush, B5 self-detach), `SyntheticWorkTests` (A5, C3 — pure C++, fastest), and `LifetimeTests` — the big win, because calling `Scene` APIs directly makes A2/A3 scriptable without a mouse (drive `startGhostEdge`/`clear`/`prepareForShutdown` programmatically). Exit code = failure count → CI/`ctest`-ready, no window flicker. Est. ~300-400 lines + one CMake block.
- **Tier 3 — manual GUI checklist:** the Layer-3 items in §2.

### 4. Open work (deferred by scope decision)

- **Pass 4 remainder:** full unification of the three mutation paths through the command layer (script mutations remain non-undoable by decision; JS listeners only see facade-path events — documented limitation).
- **Pass 5 (UX & hygiene):** shortcut conflicts (Ctrl+1 double-bound; five unconnected menu actions; triplicated Delete path), palette click wiring (`nodeCreationRequested` never connected), zoom clamp, two disagreeing grid systems (View 20 px vs Scene 40), logging diet (`QLoggingCategory`, guard the "temporary" instance counters), dead code (stub loaders, `PaletteButton`, `removeNode/removeEdge`, `snapPoint`, `createEdge/createXmlNode`, scratch XML doc), comment reconciliation, `Node::read` x/y leak, template `{{ID}}`/`{{X}}` placeholders, libxml2 `XML_PARSE_NONET` + error detail, autosave UTF-8 codec + atomic write.
- **Documented limitations (not bugs, by design decision):** multi-instance statics (`s_clearingGraph`, `s_batchDepth`, `ScriptedNode::s_engine`) unchanged; Duktape backend compiles only under `ENABLE_DUKTAPE=ON` (C4 verified by reading, not execution).
- **Optional:** one-off `-DENABLE_DUKTAPE=ON` configure to compile-check C4; Tier-2 test app.

### 5. Repository state

The committed tree contains BOTH this remediation and previously uncommitted work from the earlier session (the `ScriptEngine` seam files `script_engine.h`, `qjs_script_backend.*`, `duktape_script_backend.*`, `undo_commands.*`, plus facade/palette modifications predating this review). Per owner direction both workstreams are committed together; this entry is the dividing record of what the remediation changed.
