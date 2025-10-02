# UI Improvements & Code Coverage Analysis

**Date**: 2025-10-02
**Branch**: `feat/graph-rearch-01`

## Current UI Status

### Existing Features (Already Implemented)

**Node Palette Widget** (node_palette_widget.h/cpp):
- ✅ Icon-based grid layout with search filtering
- ✅ Drag-and-drop node creation from palette
- ✅ Double-click to create nodes
- ✅ Professional node type icons
- ✅ Tooltip descriptions for each node type
- ✅ NodeTemplate structure for extensibility

**Professional UI Components**:
- ✅ Multi-section status bar (file info, graph stats, selection, position, zoom)
- ✅ Dockable node palette widget
- ✅ Menu system (File, Edit, View, Tools, Help)
- ✅ Keyboard shortcuts (Ctrl+1/2/3 for node creation, Ctrl+S/L for save/load)
- ✅ Progress bar for long operations

**Window Features** (window.h/cpp):
- ✅ Professional menus with actions
- ✅ Autosave observer with 750ms debounce
- ✅ JavaScript engine integration
- ✅ View zooming (Ctrl+/-, Ctrl+0, Ctrl+F)

---

## UI Improvements from Other Branches

### Branch Survey Results

**Checked Branches**:
1. `origin/feature/drag-drop-palette` - Already integrated ✅
2. `origin/feature/enhanced-socket-visual-feedback` - Partial integration
3. `origin/feature/context-menu-nodes` - Replaced by palette system
4. `origin/feature/improve-connection-ux` - Potential improvements

### Candidate UI Improvements

#### 1. Enhanced Socket Visual Feedback

**Branch**: `origin/feature/enhanced-socket-visual-feedback`
**Commits**:
- `a7c51cf` - Complete socket visual feedback enhancements with dynamic node sizing
- `23e8ed6` - Enhance socket and edge visual feedback system

**Features**:
- Socket state highlighting (idle, hover, connecting, connected)
- Dynamic socket sizing based on connection state
- Enhanced edge rendering with smooth curves
- Visual feedback during ghost edge drag

**Status**: Partially implemented, needs review and integration

**Integration Effort**: Medium (2-3 hours)
- Review socket.h/cpp for visual state changes
- Check edge.h/cpp for rendering improvements
- Test with existing ghost edge system
- Ensure no conflicts with current architecture

---

#### 2. Context Menu System (Alternative to Palette)

**Branch**: `origin/feature/context-menu-nodes`
**Commits**:
- `06477e9` - Complete working context menu system
- `386fb1b` - Implement Shift+Left-click context menu

**Features**:
- Shift+Left-click opens node creation menu at cursor position
- Quick node creation without palette
- Context-aware menu options

**Status**: **NOT RECOMMENDED** - Already replaced by superior drag-drop palette

**Reasoning**:
- Current palette system is more discoverable
- Drag-drop provides visual feedback
- Icons make node types recognizable
- Context menu was deemed "destabilizing" (commit `4fb2aac`)

---

#### 3. Connection UX Improvements

**Branch**: `origin/feature/improve-connection-ux`
**Commits**: Multiple UX refinements

**Features**:
- Enhanced visual selection highlighting
- Automatic but visual smoke tests
- Template edge connection testing

**Status**: Needs detailed review

**Integration Effort**: Low-Medium (1-2 hours)
- Extract visual highlighting improvements
- Review smoke test infrastructure (may be useful for validation)
- Check for any connection logic improvements

---

#### 4. Edge Visual Improvements

**Branches**:
- `origin/feature/fix-edge-outline`
- `origin/feature/fix-edge-bounding-box`
- `origin/fix/edge-bezier-fill-issue`

**Features**:
- Fixed edge bounding box calculations
- Improved Bezier curve rendering
- Edge outline styling

**Status**: Needs investigation

**Integration Effort**: Low (30min - 1 hour)
- Review edge.cpp rendering code
- Check if issues are already fixed in current branch
- Test edge selection and bounding box behavior

---

## LLVM Code Coverage Analysis

### Available Tools (Installed)

```bash
$ which llvm-cov clang++ llvm-profdata
/usr/bin/llvm-cov      # Coverage visualization
/usr/bin/clang++       # Compiler with coverage support
/usr/bin/llvm-profdata # Coverage data merging

$ llvm-cov --version
Ubuntu LLVM version 14.0.0
```

### Coverage Workflow

#### Step 1: Instrument Code for Coverage

Modify `CMakeLists.txt` to add coverage flags:

```cmake
# Add coverage build option
option(ENABLE_COVERAGE "Enable code coverage" OFF)

if(ENABLE_COVERAGE)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-instr-generate -fcoverage-mapping")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fprofile-instr-generate")
        message(STATUS "Code coverage enabled (Clang)")
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --coverage")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage")
        message(STATUS "Code coverage enabled (GCC)")
    else()
        message(WARNING "Code coverage not supported for compiler: ${CMAKE_CXX_COMPILER_ID}")
    endif()
endif()
```

#### Step 2: Build with Coverage

```bash
# Clean build with coverage enabled
./build.sh debug clean
cd build_linux
cmake -DENABLE_COVERAGE=ON ..
make -j$(nproc)
```

#### Step 3: Run Tests to Generate Coverage Data

```bash
# Set environment variable for Clang coverage output
export LLVM_PROFILE_FILE="nodegraph-%p.profraw"

# Run application with test script
./NodeGraph --test

# Or run specific JavaScript tests
./NodeGraph  # then load scripts/test_qgraph_state_tracking.js
```

#### Step 4: Merge Coverage Data (Clang)

```bash
# Merge all .profraw files
llvm-profdata merge -sparse nodegraph-*.profraw -o nodegraph.profdata
```

#### Step 5: Generate Coverage Reports

**HTML Report** (most useful):
```bash
llvm-cov show ./NodeGraph \
    -instr-profile=nodegraph.profdata \
    -format=html \
    -output-dir=coverage_html \
    -Xdemangler c++filt \
    -show-line-counts-or-regions \
    -show-instantiations
```

**Text Summary**:
```bash
llvm-cov report ./NodeGraph \
    -instr-profile=nodegraph.profdata \
    -use-color
```

**Line-by-line for specific file**:
```bash
llvm-cov show ./NodeGraph \
    -instr-profile=nodegraph.profdata \
    qgraph.cpp \
    -use-color
```

#### Step 6: View Results

```bash
# Open HTML coverage report
firefox coverage_html/index.html
# or
google-chrome coverage_html/index.html
```

---

### Coverage Analysis Strategy

**Phase 1: Baseline Coverage (Current State)**

Measure coverage of existing code without adding tests:
1. Run application manually, exercise UI features
2. Run existing JavaScript test scripts
3. Generate baseline coverage report
4. Identify completely untested files/functions

**Expected Results**:
- Low coverage (~5-10%) - mostly initialization code
- No unit tests, only integration tests via UI/JavaScript
- identify critical paths with zero coverage

**Phase 2: Targeted Test Creation**

Focus on critical untested code:
1. **QGraph operations**: createNode, connect, deleteNode, loadXml
2. **Scene management**: addNode, addEdge, registry operations
3. **GraphFactory**: XML parsing, phased loading, edge resolution
4. **JavaScript integration**: Q_INVOKABLE methods, signal connections
5. **Observer pattern**: Batching, notification, autosave triggers

**Phase 3: Coverage-Driven Development**

Use coverage reports to guide test creation:
1. Generate coverage report after each test script
2. Identify uncovered branches (if/else, switch cases)
3. Create JavaScript tests targeting uncovered code
4. Iterate until critical paths have >80% coverage

---

### Existing Coverage Analysis Tool

**File**: `analyze_test_coverage.py`
**Uses**: libclang Python bindings
**Functionality**: Static analysis to find public methods without tests

**Limitations**:
- Static analysis only (doesn't measure actual execution)
- Can't detect coverage of specific lines/branches
- Useful for finding completely untested functions

**Recommendation**: Use LLVM coverage tools for dynamic analysis, keep Python script for quick static checks

---

## Implementation Plan

### UI Improvements Priority

**High Priority** (Do First):
1. ✅ Drag-drop palette - Already done
2. ⏳ Enhanced socket visual feedback - Review and integrate
3. ⏳ Edge bounding box fixes - Quick win, improves visual polish

**Medium Priority**:
4. Connection UX improvements - Extract useful parts
5. Visual smoke tests - Add to test suite

**Low Priority** (Defer):
6. Context menu system - Redundant with palette

---

### Coverage Analysis Priority

**Phase 1** (This Week):
1. Add coverage build option to CMakeLists.txt
2. Build with coverage instrumentation
3. Run existing JavaScript tests
4. Generate baseline HTML coverage report
5. Document untested critical paths

**Phase 2** (Next Week):
1. Create JavaScript test suite targeting QGraph
2. Create test for GraphFactory phased loading
3. Create test for observer pattern batching
4. Generate new coverage report, measure improvement

**Phase 3** (Ongoing):
1. Add coverage check to CI/CD (if implemented)
2. Set coverage threshold (e.g., >70% for core classes)
3. Regular coverage reporting

---

## Next Steps

**Immediate Actions** (Today):
1. Review `origin/feature/enhanced-socket-visual-feedback` branch
   - `git diff feat/graph-rearch-01...origin/feature/enhanced-socket-visual-feedback -- socket.cpp socket.h`
   - Identify visual feedback improvements
   - Cherry-pick useful commits

2. Add LLVM coverage support to build system
   - Edit CMakeLists.txt (add ENABLE_COVERAGE option)
   - Test coverage build
   - Generate first baseline report

**Short-term Actions** (This Week):
1. Integrate socket visual feedback improvements
2. Create coverage analysis report
3. Write JavaScript test suite for uncovered QGraph methods
4. Document coverage workflow in LINUX_BUILD.md

**Long-term Actions** (Next Sprint):
1. Achieve >70% coverage for QGraph, Scene, GraphFactory
2. Create visual regression tests (screenshot comparison)
3. Add coverage badge to README
4. Set up automated coverage reporting

---

## Technical Notes

### Coverage Build Considerations

**Clang vs GCC**:
- Clang: Uses `-fprofile-instr-generate -fcoverage-mapping`
- GCC: Uses `--coverage` (equivalent to `-fprofile-arcs -ftest-coverage`)
- Prefer Clang for better LLVM integration

**Performance Impact**:
- Coverage instrumentation adds ~10-20% overhead
- Only enable for debug/test builds
- Never enable in release builds

**Data File Locations**:
- Clang: Generates `.profraw` files (binary)
- GCC: Generates `.gcda` files (binary)
- Use `LLVM_PROFILE_FILE` env var to control output location

### Integration with Qt

**Qt-Specific Considerations**:
- QObject methods may show lower coverage (signal/slot connections)
- QGraphicsItem rendering methods require visual tests
- Mock QGraphicsScene for unit tests to avoid UI dependency

**JavaScript Coverage**:
- LLVM coverage measures C++ code executed by JavaScript
- JavaScript test scripts indirectly test C++ via Q_INVOKABLE methods
- Consider using JavaScript coverage tools (e.g., Istanbul) for .js files

---

## References

**LLVM Coverage Documentation**:
- https://clang.llvm.org/docs/SourceBasedCodeCoverage.html
- https://llvm.org/docs/CommandGuide/llvm-cov.html
- https://llvm.org/docs/CommandGuide/llvm-profdata.html

**Qt Testing**:
- Qt Test Framework: https://doc.qt.io/qt-5/qtest-overview.html
- QTest with coverage: Combine QTest unit tests with LLVM coverage

**Branch Analysis**:
- `git log --oneline --all --graph` - Visualize branch relationships
- `git diff branch1...branch2 -- file` - Compare specific files between branches
- `git cherry-pick <commit>` - Apply specific commits from other branches
