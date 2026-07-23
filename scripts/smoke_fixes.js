// scripts/smoke_fixes.js
// Functional verification for the Phase A/B/C fixes (ADVISORY.md Entry 003).
// Run from repo root:
//   PATH="/d/Qt/5.15.19/debug/bin:$PATH" ./build_Debug/Debug/NodeGraph.exe --script scripts/smoke_fixes.js
// Results: one "TEST <name> PASS/FAIL" line per check in logs/NodeGraph_*.log
// Note: console.log glue takes ONE argument - always pass a single string.

var passes = 0, fails = 0;
function check(name, cond, detail) {
    if (cond) { passes++; console.log("TEST " + name + " PASS"); }
    else      { fails++;  console.log("TEST " + name + " FAIL " + (detail || "")); }
}
function counts() { var s = graph.getGraphStats(); return [s.nodeCount, s.edgeCount]; }

console.log("=== SMOKE FIXES START ===");
console.log("available types: " + graph.getAvailableNodeTypes().join(","));

// ---------- A1 + C6: pass-through round-trip; script load REPLACES ----------
var src  = graph.createNode("SOURCE", 0, 0);
var tr   = graph.createNode("TRANSFORM", 250, 0);
var snk  = graph.createNode("SINK", 500, 0);
var e1 = graph.connectNodes(src, 0, tr, 0);
var e2 = graph.connectNodes(tr, 0, snk, 0);
check("setup.chain", e1 !== "" && e2 !== "", "e1=" + e1 + " e2=" + e2);

var saved = graph.saveToFile("logs/smoke_roundtrip.xml");
check("A1.save", saved === true, "saveToFile returned " + saved);

graph.clearGraph();
var c0 = counts();
check("setup.cleared", c0[0] === 0 && c0[1] === 0, "counts=" + c0.join("/"));

// Stray exists ONLY in the live scene (never saved): a true replace wipes it,
// a merge would keep it (4 nodes instead of 3).
var stray = graph.createNode("SINK", 900, 900);

var loaded = graph.loadFromFile("logs/smoke_roundtrip.xml");
var c1 = counts();
check("A1.load.passthrough", loaded === true && c1[0] === 3 && c1[1] === 2,
      "loaded=" + loaded + " counts=" + c1.join("/") + " (was rejected pre-fix)");
check("C6.load.replaces", c1[0] === 3, "stray node merged? count=" + c1[0]);

// ---------- B2: duplicate output-socket connect refused cleanly ----------
var extra = graph.createNode("SINK", 700, 300);
var edgesBefore = counts()[1];
var dup = graph.connectNodes(src, 0, extra, 0);   // src out0 already feeds tr
check("B2.dup.refused", dup === "" && counts()[1] === edgesBefore,
      "dup=" + dup + " edges=" + counts()[1] + " vs " + edgesBefore);
graph.deleteNode(extra);

// ---------- A5: durationMs is actually measured ----------
var work = graph.runSyntheticWork({task: "loop", iterations: 200000});
check("A5.durationMs", work.durationMs > 0, "durationMs=" + work.durationMs);

// ---------- C3: delay clamp (999999ms -> 5000ms) ----------
var t0 = new Date().getTime();
var d = graph.runSyntheticWork({task: "delay", delayMs: 999999});
var clampElapsed = new Date().getTime() - t0;
check("C3.delay.clamped", d.delayMs === 5000 && clampElapsed < 9000,
      "delayMs=" + d.delayMs + " elapsed=" + clampElapsed);

// ---------- B3: a script cannot delete its own node ----------
graph.setNodeScript(tr, "graph.deleteNode(node.nodeId()); 'deleted'");
graph.executeNodeScript(tr, {});
check("B3.selfdelete.refused", graph.getAllNodes().indexOf(tr) !== -1,
      "node missing after self-delete attempt");

// ---------- C1: recursive executeNodeScript is depth-capped ----------
graph.setNodeScript(tr, "graph.executeNodeScript(node.nodeId(), {}); 'done'");
graph.executeNodeScript(tr, {});
check("C1.recursion.capped", graph.getAllNodes().indexOf(tr) !== -1,
      "app/node did not survive recursion");

// ---------- C5: facade batch is the real mechanism now ----------
graph.beginBatch();
var inBatch = graph.isBatchMode();
var batched = graph.createNode("SINK", 100, 400);
graph.endBatch();
check("C5.batch.real", inBatch === true && graph.isBatchMode() === false,
      "inBatch=" + inBatch + " after=" + graph.isBatchMode());
graph.deleteNode(batched);

// ---------- C7: toXml() returns real XML ----------
var xml = graph.toXml();
check("C7.toXml.real", xml.length > 40 && xml.indexOf("<graph") !== -1 && xml !== "<graph></graph>",
      "len=" + xml.length);

console.log("SMOKE DONE PASS=" + passes + " FAIL=" + fails);
console.log("=== SMOKE FIXES END ===");
