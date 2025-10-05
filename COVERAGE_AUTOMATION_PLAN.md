# Coverage-Driven Test Automation Plan

## EN — Plan for an iterative "coverage → hammer → learn" loop (Qt5)

### 0) Objective & Scope

**Goal:** Continuously raise coverage of a Qt5 C++ graph application by finding uncovered regions and driving the app via JavaScript to exercise those paths.

**Scope:** Build config, coverage export, JSON analysis, JS hammering, and an automated iteration loop (local + CI).

### 1) Assumptions & Interfaces

- Build system is Qt5 (CMake or qmake).
- App can evaluate a JavaScript file (via QJSEngine) and expose a Graph-like API (create/connect/delete nodes, save/load).
- We can run the app headless (offscreen) for automation.

### 2) Artifacts (high-level)

- **Coverage build & export script** (already have 1–2; extend to produce machine-readable coverage JSON alongside HTML).
- **Uncovered analysis:** a tool that reads coverage JSON and identifies files/functions/line ranges to target.
- **JavaScript hammer spec:** a generator that creates targeted JS scenarios to exercise undercovered areas.
- **Orchestrator:** a loop that (a) generates targeted JS, (b) runs the app with it, (c) re-merges coverage, (d) re-exports JSON, (e) checks progress, (f) repeats.

### 3) Baseline & Export (Phase A)

- Ensure instrumented build exists (clang with coverage flags).
- Ensure coverage export produces:
  - profile data (merged)
  - JSON (machine-readable; prefer JSON, fallback acceptable)
  - HTML report for humans
- Ensure a list of relevant binaries/DSOs is captured for downstream use (so exports include all Qt plugins/modules).

**Acceptance:** Export step reliably produces a current coverage.json for each run.

### 4) Coverage Analysis (Phase B)

- Parse coverage.json and compute per-file coverage %.
- Identify target files using thresholds (e.g., files <95% or with any uncovered regions).
- Tag each target file with action categories via filename/path heuristics (e.g., Graph, Node, Edge, XML/persistence, JS engine, error paths).
- Optionally sort by lowest coverage first and cap to a manageable set per iteration.

**Acceptance:** Output a compact per-iteration target set with tags and simple rationale (why each file is targeted).

### 5) JavaScript Hammer Generation (Phase C)

From the target set, generate a JS scenario that:

- Exercises graph basics (create/connect/delete nodes; save/load).
- Triggers XML/persistence actions (multiple saves/loads and round-trips).
- Exercises JS engine paths (assign node scripts, evaluate them with various inputs).
- Provokes edge/path corner cases (duplicate connects, mismatched sockets if allowed).

The scenario should fail gracefully (log and continue) when an API isn't present—no hard aborts.

Keep it parameterized (node types list, iteration counts) so we can grow coverage progressively.

**Acceptance:** The generated JS runs to completion against the app, mutates the graph meaningfully, and is resilient to missing APIs.

### 6) Orchestrated "Correctio" Loop (Phase D)

Loop steps per iteration:

1. Analyze current coverage JSON → produce/update target set.
2. Generate JS scenario biased to those targets.
3. Run the app with the JS (headless), with profiling env set to emit fresh data.
4. Merge new profiles; export fresh JSON; optionally summarize overall %.
5. Decide: if coverage >= target threshold or gains flatten, end; else continue.
6. Produce per-iteration artifact bundle (JSON, report, short log, summary metrics).

**Acceptance:** A single command (or CI job) performs N iterations and stops at the defined threshold, storing artifacts.

### 7) Growth & Refinement (Phase E)

- **Biasing:** Use the previous iteration's dark files to shape next-round JS (e.g., pick node types/operations that historically light up specific modules).
- **New behaviors:** As new node types/features land, add hammer actions (e.g., undo/redo, invalid XML, recovery paths).
- **Noise control:** Maintain filters to avoid spending cycles on generated code, third-party libs, or test harness internals.

**Acceptance:** Each sprint can add 1–2 new hammer actions tied to actual dark modules, improving line or region coverage by measurable amounts.

### 8) Reporting & Quality Gates (Phase F)

- **Dashboards:** record coverage %, undercovered hotspots, and "new dark code" introduced since last run.
- **Gates:** optional PR check: fail if module coverage regresses beyond tolerance, or if new files < X% coverage appear.
- **Narrative notes:** keep a changelog of what the hammer learned, what actions were added, and the impact.

**Acceptance:** Stakeholders can see trendlines; PRs can be gated; developers know where to focus.

### 9) CI Integration (Phase G)

- Nightly or on-demand job that runs 1–N iterations (fewer in PRs, more nightly).
- Artifacts persisted (coverage JSON, HTML, target lists, JS scenarios, metrics).
- Headless Qt configured on CI runners.

**Acceptance:** Green CI path for normal PRs; nightly tasks improve breadth.

### 10) Risks & Mitigations

- **Flaky UI paths:** Prefer headless flows and deterministic graph ops.
- **Heuristic misfires:** Keep the generator conservative; add per-module opt-outs or custom action packs.
- **Performance:** Cap iteration count; rotate targets per run.
- **Data drift:** Re-build the DSOs/binary list each baseline export to keep llvm-cov accurate.

---

## Implementation Status

### Phase A (Baseline & Export) - 80% Complete ✅

**Completed:**
- ✅ Instrumented build exists (`coverage.sh` with Clang)
- ✅ Profile merge works (`.profraw` → `.profdata`)
- ✅ HTML report generated

**Missing:**
- ❌ JSON export (`llvm-cov export -format=text > coverage.json`)

**Files:**
- `coverage.sh` - Build, test, and report script
- `CMakeLists.txt` - Has `ENABLE_COVERAGE` option for Clang

### Phase B (Coverage Analysis) - Not Started ❌

**Need to build:**
- Parse `coverage.json` and compute per-file coverage %
- Identify targets (<95% threshold)
- Tag files by category (Graph/Node/Edge/XML/JS)

**Suggested file:**
- `analyze_coverage_gaps.py` - Parse JSON, output target list

### Phase C (JS Hammer Generation) - Not Started ❌

**What we have:**
- JavaScript engine with Graph API (from earlier work)
- Basic test scripts in `scripts/` directory

**Need to build:**
- Template-based scenario generator
- Targeted JS generation based on coverage gaps

**Suggested file:**
- `generate_hammer_tests.py` - Create JS tests targeting uncovered code

### Phase D (Orchestrated Loop) - Not Started ❌

**Need to build:**
- Main orchestrator that runs N iterations
- Iteration artifact storage
- Convergence detection

**Suggested file:**
- `coverage_loop.sh` - Main orchestration script

### Phases E-G - Future Enhancements

---

## Next Steps

1. **Complete Phase A:** Add JSON export to `coverage.sh`
2. **Build Phase B:** Create coverage gap analyzer
3. **Build Phase C:** Create JS test generator
4. **Build Phase D:** Create orchestration loop
5. **Iterate:** Run and refine based on results
