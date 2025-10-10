# Test Coverage Improvement Plan

**Date:** 2025-10-09
**Branch:** feat/improve-test-coverage
**Goal:** Increase coverage from 44% → 80%+ (lines)

---

## 📊 Current State Analysis

### Coverage Baseline (from coverage_analysis.json)
- **Lines:** 44.44% (1834/4127 covered)
- **Functions:** 43.10% (150/348 covered)
- **Regions:** 37.19% (688/1850 covered)
- **Branches:** 24.64% (304/1234 covered)

### Critical Findings
1. **GUI-Only Code:** ghost_edge.cpp (0% coverage) - Interactive edge creation, requires GUI tests
2. **Biggest Gaps:** qgraph.cpp (345 lines), window.cpp (291 lines), scene.cpp (256 lines)
3. **Current Test:** Only loads XML and exits (happy path smoke test, no interactive UI)

---

## 🎯 Coverage Targets

| Component | Current | Target | Strategy |
|-----------|---------|--------|----------|
| **qgraph.cpp** | 22% | 85% | API unit tests |
| **javascript_engine.cpp** | 55% | 85% | JS API test suite |
| **node_templates.cpp** | 19% | 80% | Template system tests |
| **graph_factory.cpp** | 31% | 80% | Error injection tests |
| **edge.cpp** | 60% | 85% | Edge validation tests |
| **socket.cpp** | 39% | 75% | Connection tests |
| **graph_observer.cpp** | 40% | 75% | Observer pattern tests |
| **xml_autosave_observer.cpp** | 66% | 80% | I/O error tests |
| **node.cpp** | 69% | 85% | Lifecycle tests |

**UI Components (lower priority):**
- ghost_edge.cpp: 0% → 60% (interactive edge creation - requires mouse simulation)
- scene.cpp: 16% → 50% (GUI interactions - mouse events, ghost edge)
- window.cpp: 50% → 60% (menu/dialog testing complex)
- view.cpp: 37% → 50% (viewport interactions)
- node_palette_widget.cpp: 64% → 70% (already decent)

---

## 📋 Implementation Plan

### Phase 1: Infrastructure Setup (Day 1)
**Estimated Time:** 3-4 hours

#### 1.1 Restore File-Based Logging System
**Why:** Enables detailed execution tracing for test debugging and coverage verification
- Add `setupLogging()` function to main.cpp (from commit history)
- Redirect all qDebug() to timestamped log files: `logs/NodeGraph_YYYY-MM-DD_hh-mm-ss.log`
- Include timestamps with milliseconds, severity levels (DEBUG/INFO/WARN/ERROR/FATAL)
- Add session ID tracking for log correlation
- Add logging to ghost_edge.cpp (constructor, setPath, paint, destructor)
- **Benefit:** Can verify code execution by checking log files
- **Impact:** Makes debugging test failures much easier

#### 1.2 Commit Coverage Baseline
- Commit coverage analysis artifacts:
  - coverage_analysis.json
  - coverage_summary.txt
  - analyze_coverage.py
  - coverage_llvm_wsl.sh (updated with Qt path)
  - LOGGING_SYSTEM_RESTORE.md
- Create this plan document
- **Branch:** Create `feat/improve-test-coverage` from `fix/clean-main`

#### 1.3 Test Infrastructure Setup
- **CRITICAL:** No separate test executables - only the main NodeGraph application exists
- Create JavaScript test scripts in `test_scripts/` directory for API testing
- Add command-line test mode to main.cpp (--run-tests flag)
- Create test XML fixtures in root directory

---

### Phase 2: QGraph Facade Testing (Day 1-2)
**Estimated Time:** 6-8 hours
**Target:** qgraph.cpp 22% → 85% (+345 lines = +7.8% overall)

#### 2.1 Create QGraph Test Suite
**File:** `tests/unit/test_qgraph.cpp`

**Test Cases:**
```cpp
class TestQGraph : public QObject {
    Q_OBJECT
private slots:
    // Basic API tests
    void test_loadXml_valid();           // ✓ Already covered
    void test_loadXml_empty();
    void test_loadXml_malformed();
    void test_loadXml_missing_nodes();
    void test_loadXml_invalid_edges();
    void test_loadXml_circular_refs();

    // Save/Export tests
    void test_saveXml_basic();
    void test_saveXml_empty_graph();
    void test_saveXml_complex_graph();
    void test_saveXml_roundtrip();

    // Graph manipulation
    void test_addNode_basic();
    void test_addNode_invalid_type();
    void test_deleteNode_basic();
    void test_deleteNode_with_edges();
    void test_deleteNode_nonexistent();

    void test_addEdge_valid();
    void test_addEdge_invalid_sockets();
    void test_addEdge_duplicate();
    void test_addEdge_self_loop();
    void test_deleteEdge_basic();

    void test_clearGraph();
    void test_clearGraph_with_observers();

    // Query APIs
    void test_findNode_by_id();
    void test_getConnections();
    void test_getNodeCount();
    void test_getEdgeCount();

    // Error handling
    void test_error_null_pointer();
    void test_error_invalid_state();
};
```

**Coverage Impact:** ~280-320 lines

---

### Phase 3: JavaScript Engine Testing (Day 2-3)
**Estimated Time:** 4-6 hours
**Target:** javascript_engine.cpp 55% → 85% (+150 lines = +3.4% overall)

#### 3.1 Create JS API Test Suite
**File:** `tests/unit/test_javascript_engine.cpp`

**Test Strategy:** Execute JavaScript code that calls exposed APIs

**Test Cases:**
```cpp
class TestJavaScriptEngine : public QObject {
    Q_OBJECT
private slots:
    // Graph algorithm tests
    void test_js_dfs_traversal();
    void test_js_bfs_traversal();
    void test_js_topological_sort();
    void test_js_cycle_detection();
    void test_js_shortest_path();

    // Node manipulation
    void test_js_get_node();
    void test_js_set_node_property();
    void test_js_get_node_connections();

    // Edge traversal
    void test_js_get_edges();
    void test_js_follow_edge();
    void test_js_get_neighbors();

    // Graph queries
    void test_js_node_count();
    void test_js_edge_count();
    void test_js_is_connected();
    void test_js_is_cyclic();

    // Error handling
    void test_js_invalid_syntax();
    void test_js_runtime_error();
    void test_js_type_error();
};
```

**Test Scripts:** Create `.js` files in `tests/fixtures/scripts/`

**Coverage Impact:** ~150-180 lines

---

### Phase 4: Node Templates & Factory Testing (Day 3)
**Estimated Time:** 3-4 hours
**Target:** node_templates.cpp 19% → 80%, graph_factory.cpp 31% → 80%

#### 4.1 Node Templates Tests
**File:** `tests/unit/test_node_templates.cpp`

```cpp
class TestNodeTemplates : public QObject {
    Q_OBJECT
private slots:
    // Built-in templates
    void test_builtin_templates_loaded();
    void test_template_SOURCE();
    void test_template_SINK();
    void test_template_TRANSFORM();
    void test_template_MERGE();
    void test_template_SPLIT();

    // Custom templates
    void test_register_custom_template();
    void test_custom_template_validation();
    void test_template_override();

    // Template operations
    void test_clone_template();
    void test_serialize_template();
    void test_deserialize_template();

    // Validation
    void test_validate_socket_config();
    void test_invalid_template_reject();
};
```

**Coverage Impact:** ~80-100 lines

#### 4.2 Graph Factory Error Tests
**File:** `tests/unit/test_graph_factory.cpp`

```cpp
class TestGraphFactory : public QObject {
    Q_OBJECT
private slots:
    // Error injection tests
    void test_load_corrupt_xml();
    void test_load_missing_attributes();
    void test_load_invalid_node_type();
    void test_load_dangling_edges();
    void test_load_duplicate_ids();
    void test_load_version_mismatch();

    // Validation tests
    void test_validate_node_structure();
    void test_validate_edge_structure();
    void test_validate_socket_bounds();
};
```

**Coverage Impact:** ~50-60 lines

---

### Phase 5: Core Component Testing (Day 4)
**Estimated Time:** 4-5 hours
**Target:** edge.cpp, socket.cpp, node.cpp

#### 5.1 Edge Tests
**File:** `tests/unit/test_edge.cpp`

```cpp
class TestEdge : public QObject {
    Q_OBJECT
private slots:
    // Edge lifecycle
    void test_create_edge();
    void test_connect_sockets();
    void test_disconnect_edge();
    void test_delete_edge();

    // Validation
    void test_prevent_duplicate_connection();
    void test_prevent_self_loop();
    void test_prevent_invalid_socket();

    // Visual/Path
    void test_edge_path_calculation();
    void test_edge_curve_controls();
    void test_edge_hit_testing();

    // Serialization
    void test_edge_to_xml();
    void test_edge_from_xml();
};
```

**Coverage Impact:** ~120-150 lines

#### 5.2 Socket Tests
**File:** `tests/unit/test_socket.cpp`

```cpp
class TestSocket : public QObject {
    Q_OBJECT
private slots:
    // Connection management
    void test_socket_connect();
    void test_socket_disconnect();
    void test_socket_is_connected();
    void test_socket_multiple_edges();

    // Validation
    void test_connection_type_check();
    void test_connection_arity_limit();
    void test_prevent_duplicate_edge();

    // Positioning
    void test_socket_position_update();
    void test_socket_scene_position();
};
```

**Coverage Impact:** ~80-100 lines

#### 5.3 Node Lifecycle Tests
**File:** `tests/unit/test_node.cpp`

```cpp
class TestNode : public QObject {
    Q_OBJECT
private slots:
    // Lifecycle
    void test_create_node();
    void test_delete_node();
    void test_delete_node_with_edges();

    // Properties
    void test_set_node_position();
    void test_set_node_label();
    void test_get_node_sockets();

    // Validation
    void test_validate_node_state();
    void test_node_bounds();
};
```

**Coverage Impact:** ~90-110 lines

---

### Phase 6: Observer & Autosave Testing (Day 4)
**Estimated Time:** 2-3 hours
**Target:** graph_observer.cpp, xml_autosave_observer.cpp

#### 6.1 Observer Pattern Tests
**File:** `tests/unit/test_graph_observer.cpp`

```cpp
class TestGraphObserver : public QObject {
    Q_OBJECT
private slots:
    // Observer lifecycle
    void test_attach_observer();
    void test_detach_observer();
    void test_multiple_observers();

    // Notification tests
    void test_notify_graph_cleared();
    void test_notify_node_added();
    void test_notify_edge_added();

    // Batch mode
    void test_batch_mode_suppression();
    void test_batch_mode_nesting();
    void test_batch_mode_end_notify();
};
```

#### 6.2 Autosave Tests
**File:** `tests/unit/test_xml_autosave.cpp`

```cpp
class TestXmlAutosave : public QObject {
    Q_OBJECT
private slots:
    // Autosave triggering
    void test_autosave_on_change();
    void test_autosave_timer();
    void test_autosave_batch_skip();

    // Error handling
    void test_autosave_write_failure();
    void test_autosave_invalid_path();
    void test_autosave_disk_full();

    // State management
    void test_dirty_state_tracking();
    void test_autosave_cancellation();
};
```

**Coverage Impact:** ~80-100 lines combined

---

### Phase 7: GUI Interaction Testing (Day 5) - OPTIONAL
**Estimated Time:** 4-6 hours
**Target:** ghost_edge.cpp 0% → 60%, scene.cpp mouse events

#### 7.1 Ghost Edge & Scene Interaction Tests
**File:** `tests/gui/test_scene_interaction.cpp`

**Test Strategy:** Use QTest::mousePress, QTest::mouseMove, QTest::mouseRelease

**Test Cases:**
```cpp
class TestSceneInteraction : public QObject {
    Q_OBJECT
private slots:
    // Ghost edge lifecycle
    void test_start_ghost_edge();
    void test_update_ghost_edge_path();
    void test_finish_ghost_edge_valid();
    void test_cancel_ghost_edge();

    // Interactive edge creation
    void test_right_click_drag_create_edge();
    void test_magnetic_socket_snapping();
    void test_ghost_edge_visual_feedback();
    void test_invalid_connection_reject();

    // Mouse event handling
    void test_scene_mouse_move_with_ghost();
    void test_scene_mouse_release_complete();
    void test_scene_escape_cancel();

    // Socket highlighting
    void test_socket_hover_highlight();
    void test_valid_target_indication();
};
```

**Why ghost_edge.cpp has 0% coverage:**
- Used **only** during interactive edge creation (right-click drag from socket)
- Current test loads XML with pre-defined edges (no interactive creation)
- Requires mouse event simulation to exercise
- Active code, not dead code - scene.cpp, socket.cpp, qgraph.cpp all call it

**Coverage Impact:** ~20-26 lines (ghost_edge.cpp) + ~60 lines (scene.cpp mouse handlers)

---

## 🧪 Test Execution Strategy

### Test Runner Script
**File:** `tests/run_all_tests.sh`

```bash
#!/bin/bash
set -e

# Build tests
cmake -S . -B build_test -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON
cmake --build build_test -j

# Run all test suites
for test in build_test/tests/unit/*_test; do
    echo "Running $(basename $test)..."
    $test || exit 1
done

echo "✓ All tests passed"
```

### Coverage Test Runner
**File:** `tests/run_coverage_tests.sh`

```bash
#!/bin/bash
set -euo pipefail

# Build with coverage
cmake -S . -B build_cov -DCMAKE_BUILD_TYPE=Debug -DENABLE_COVERAGE=ON -DBUILD_TESTS=ON
cmake --build build_cov -j

# Run all tests with profiling
export LLVM_PROFILE_FILE="coverage_runs/test_%p.profraw"
for test in build_cov/tests/unit/*_test; do
    echo "Running $(basename $test) with coverage..."
    $test
done

# Also run the GUI smoke test
LLVM_PROFILE_FILE="coverage_runs/gui_%p.profraw" build_cov/NodeGraph tests_medium.xml

# Merge and report
llvm-profdata merge -sparse coverage_runs/*.profraw -o coverage.profdata
llvm-cov report -instr-profile=coverage.profdata build_cov/NodeGraph build_cov/libNodeGraphCore.a
```

---

## 📈 Expected Coverage Improvements

| Phase | Lines Added | Cumulative % |
|-------|-------------|--------------|
| **Baseline** | - | 44.44% |
| Phase 1: Infrastructure | - | 44.44% |
| Phase 2: QGraph | +280 | 51.23% |
| Phase 3: JavaScript | +150 | 54.86% |
| Phase 4: Templates/Factory | +140 | 58.25% |
| Phase 5: Core Components | +280 | 65.04% |
| Phase 6: Observers | +90 | 67.22% |
| **Phase 7: GUI Interaction (optional)** | +86 | **69.30%** |
| **Stretch: Additional edge cases** | +150 | **72.93%** |

**Note:** Reaching 80%+ would require extensive UI interaction testing (window menus, dialogs, viewport operations), which is complex with Qt GUI components. 69-73% is a realistic target for unit tests + basic GUI interaction tests.

---

## 🚀 Alternative: 80%+ Target

To reach 80%, we'd need to add:
- **Scripted UI tests** using Qt Test's QTest::mouseClick, QTest::keyClick
- **QTest::qWait()** for async operations
- **Mock X11 display** for headless GUI testing

**Additional Work:**
- `tests/gui/test_scene_interaction.cpp` (+200 lines)
- `tests/gui/test_window_actions.cpp` (+250 lines)
- `tests/gui/test_view_navigation.cpp` (+80 lines)

This adds ~3-4 more days of work.

---

## 🎯 Recommended Approach

### Option A: Conservative (Recommended)
**Target:** 67-69% line coverage
**Time:** 4-5 days
**Approach:** Unit tests for all non-GUI components (Phases 1-6)
**Risk:** Low

### Option B: Moderate
**Target:** 70-73% line coverage
**Time:** 5-6 days
**Approach:** Unit tests + basic GUI interaction tests (Phases 1-7)
**Risk:** Low-Medium

### Option C: Aggressive
**Target:** 80%+ line coverage
**Time:** 8-10 days
**Approach:** Full unit + comprehensive GUI tests (all phases + window/view testing)
**Risk:** Medium (GUI tests can be flaky)

---

## 📊 Success Criteria

### Must Have
- ✅ qgraph.cpp coverage > 80%
- ✅ javascript_engine.cpp coverage > 80%
- ✅ node_templates.cpp coverage > 75%
- ✅ Overall line coverage > 65%
- ✅ All tests passing in CI
- ✅ No regressions in existing functionality

### Nice to Have
- ✅ Overall line coverage > 70%
- ✅ Branch coverage > 50%
- ✅ GUI interaction tests (ghost_edge, scene mouse events)
- ✅ Performance benchmarks
- ✅ ghost_edge.cpp coverage > 50%

---

## 🔄 Integration with Existing Tasks

This testing work complements the remaining tasks in `REMAINING_CORE_TASKS.md`:

- **Task #11 (Edge Curve Control):** Add tests in Phase 5.1
- **Task #13 (Multi-Edge Sockets):** Add tests in Phase 5.2
- **Task #12 (I/O Separation):** Add tests in Phase 4.1
- **Task #14 (CI/Style):** Integrate test runners into CI

---

## 📝 Next Steps

1. **Review this plan** with team/stakeholder
2. **Create branch:** `git checkout -b feat/improve-test-coverage`
3. **Start Phase 1:** Set up test infrastructure, commit coverage baseline
4. **Execute phases 2-6** sequentially (core unit tests)
5. **Review coverage gains** after phase 6
6. **Decide:** Proceed with Phase 7 (GUI tests) or stop at ~67% coverage
7. **Adjust strategy** if needed based on actual gains

---

## ⏱️ Time Estimate Summary

| Phase | Estimated Time |
|-------|---------------|
| Phase 1: Infrastructure + Logging | 3-4 hours |
| Phase 2: QGraph | 6-8 hours |
| Phase 3: JavaScript | 4-6 hours |
| Phase 4: Templates/Factory | 3-4 hours |
| Phase 5: Core Components | 4-5 hours |
| Phase 6: Observers | 2-3 hours |
| **Total (Option A)** | **21-29 hours (~4-5 days)** |
| Phase 7: GUI Interaction | 4-6 hours |
| **Total (Option B)** | **25-35 hours (~5-6 days)** |
| **With Full GUI tests (Option C)** | **+12-16 hours (~2-3 days more)** |

---

**Ready to proceed when approved!**
