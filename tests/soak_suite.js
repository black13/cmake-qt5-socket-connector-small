// tests/soak_suite.js
//
// Burn-in / soak run: keeps the application alive and responsive for over a
// minute while continuously mutating the graph - create/connect/delete,
// node scripts, save/reload, grid snap, topology align and undo.
//
// graph.yield(ms) pumps the event loop between batches, so timers (autosave),
// animations and the status bar keep working during the run instead of the
// UI thread being blocked for the whole script.
//
// Run interactively:
//   .\run.ps1 --script tests/soak_suite.js
// Optional CTest registration (default off, takes >1 minute):
//   cmake -DENABLE_SOAK_TEST=ON ...
//
// Exits nonzero when an invariant or round-trip check fails.

var DURATION_MS = 65000;   // run for more than a minute
var REPORT_EVERY_MS = 10000;
// Event-loop yield per iteration. Raise this (e.g. 500) to watch the soak in
// slow motion; keep it small for a maximum-throughput stress run.
var SETTLE_MS = 25;

var passes = 0;
var fails = 0;
function test(name, condition, detail) {
    if (condition) { passes++; console.log("SOAK PASS " + name); }
    else { fails++; console.log("SOAK FAIL " + name + (detail ? " (" + detail + ")" : "")); }
}
function now() { return new Date().getTime(); }
function counts() { var s = graph.getGraphStats(); return [s.nodeCount, s.edgeCount]; }
function alive(id) { return graph.getAllNodes().indexOf(id) !== -1; }

// Deterministic PRNG so a failing run can be reproduced.
var seed = 20260922;
function rnd() { seed = (seed * 1103515245 + 12345) % 2147483648; return seed / 2147483648; }
function rndInt(limit) { return Math.floor(rnd() * limit); }

var start = now();
var lastReport = 0; // relative elapsed ms, not an absolute timestamp
var iterations = 0, ops = 0, saved = 0, loaded = 0, scripts = 0;
var aligns = 0, undos = 0, deletes = 0;
var live = [];      // bookkeeping: nodes we believe are in the scene
var tail = "";      // last created node, for chaining
var peakNodes = 0;

graph.clearGraph();
console.log("SOAK start: duration=" + Math.round(DURATION_MS / 1000) + "s nodes_cap=60");

while (now() - start < DURATION_MS) {
    ++iterations;

    // 1. Add a short chain of transforms and wire it to the previous tail.
    graph.beginBatch();
    var added = 3 + rndInt(3);
    for (var i = 0; i < added; ++i) {
        var nid = graph.createNode("TRANSFORM",
            -600 + (live.length % 12) * 160,
            -400 + Math.floor(live.length / 12) * 120);
        ++ops;
        if (nid === "") {
            test("create.ok", false, "iteration " + iterations);
            break;
        }
        if (tail !== "" && graph.connectNodes(tail, 1, nid, 0) === "") {
            test("connect.ok", false, "iteration " + iterations);
        }
        ++ops;
        live.push(nid);
        tail = nid;
    }
    graph.endBatch();

    // 2. Give the tail a payload-mutating script and run it regularly.
    if (iterations % 4 === 0 && tail !== "") {
        graph.setNodeScript(tail,
            "var n = node.payloadValue('runs') || 0;" +
            "node.setPayloadValue('runs', n + 1);" +
            "return n + 1;");
        var result = graph.executeNodeScript(tail, {});
        ++scripts;
        ++ops;
        if (result === null || result === undefined || graph.getNodeScriptError(tail) !== "") {
            test("script.ok", false, "iteration " + iterations);
        }
    }

    // 3. Cap the working set by deleting the oldest nodes.
    while (live.length > 60) {
        var victim = live.shift();
        if (alive(victim)) {
            graph.deleteNode(victim);
            ++deletes;
            ++ops;
        }
    }

    // 4. Periodically prove persistence; otherwise snap/align/undo.
    if (iterations % 10 === 0) {
        var before = counts();
        if (!graph.saveToFile("logs/soak_graph.xml")) {
            test("save.ok", false, "iteration " + iterations);
        }
        ++saved;
        graph.clearGraph();
        if (!graph.loadFromFile("logs/soak_graph.xml")) {
            test("load.ok", false, "iteration " + iterations);
        }
        ++loaded;
        var after = counts();
        test("roundtrip.counts." + iterations,
             before[0] === after[0] && before[1] === after[1],
             "before=" + before.join("/") + " after=" + after.join("/"));

        // UUIDs survive the round trip; rebase bookkeeping if anything is lost
        var survivors = 0;
        for (var k = 0; k < live.length; ++k) {
            if (alive(live[k])) { ++survivors; }
        }
        if (survivors !== live.length) {
            test("roundtrip.ids." + iterations, false,
                 "survivors=" + survivors + "/" + live.length);
            live = graph.getAllNodes();
        }
        tail = live.length > 0 ? live[live.length - 1] : "";
    } else if (iterations % 5 === 0) {
        graph.setSnapToGrid(true);
        graph.snapNodes();
        ++ops;
        graph.alignGraph();
        ++aligns;
        ++ops;
        graph.undo(); // align is one undoable command
        ++undos;
        graph.setSnapToGrid(false);
    }

    peakNodes = Math.max(peakNodes, counts()[0]);

    // 5. Invariant: our bookkeeping matches the scene.
    if (counts()[0] !== live.length) {
        test("invariant.node_count", false,
             "scene=" + counts()[0] + " bookkeeping=" + live.length +
             " iteration=" + iterations);
        live = graph.getAllNodes();
    }

    // 6. Let the event loop run: autosave, animations, status updates.
    graph.yield(SETTLE_MS);

    var elapsed = now() - start;
    if (elapsed - lastReport >= REPORT_EVERY_MS) {
        lastReport = elapsed;
        console.log("SOAK t=" + Math.round(elapsed / 1000) + "s iter=" + iterations +
                    " ops=" + ops + " nodes=" + counts()[0] + " edges=" + counts()[1] +
                    " save/load=" + saved + "/" + loaded + " scripts=" + scripts +
                    " aligns=" + aligns + " deletes=" + deletes +
                    " undos=" + undos + " peak=" + peakNodes + " fail=" + fails);
    }
}

// ── Final round trip on the steady-state graph ─────────────────────────────
var finalBefore = counts();
var finalElapsed = now() - start;
test("duration.over_minute", finalElapsed >= 60000, "elapsed=" + finalElapsed + "ms");
test("final.save", graph.saveToFile("logs/soak_final.xml") === true);
graph.clearGraph();
test("final.load", graph.loadFromFile("logs/soak_final.xml") === true);
var finalAfter = counts();
test("final.counts",
     finalBefore[0] === finalAfter[0] && finalBefore[1] === finalAfter[1],
     "before=" + finalBefore.join("/") + " after=" + finalAfter.join("/"));

console.log("");
console.log("=== SOAK SUITE COMPLETE ===");
console.log("elapsed=" + Math.round(finalElapsed / 1000) + "s iterations=" + iterations +
            " ops=" + ops + " peakNodes=" + peakNodes);
console.log("PASS: " + passes + "  FAIL: " + fails);
graph.quit(fails === 0 ? 0 : 1);
