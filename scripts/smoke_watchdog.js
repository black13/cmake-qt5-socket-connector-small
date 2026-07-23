// scripts/smoke_watchdog.js
// Watchdog proof for C2 (ADVISORY.md Entry 003): an infinite node script must
// be interrupted after ~5s instead of freezing the app forever.
// Kept separate from smoke_fixes.js because the engine interrupt may also
// abort THIS outer script - the log still proves survival either way.
// Markers to look for in logs/NodeGraph_*.log:
//   "TEST C2.watchdog.armed PASS"    - always (before the spin)
//   "max script recursion" / script error about interruption
//   "TEST C2.watchdog.returned PASS" - best case: control came back

console.log("=== SMOKE WATCHDOG START ===");
var tr = graph.createNode("TRANSFORM", 250, 0);
graph.setNodeScript(tr, "while (1) {}");
console.log("TEST C2.watchdog.armed PASS - executing infinite script, expect ~5s interrupt");
var t0 = new Date().getTime();
graph.executeNodeScript(tr, {});
console.log("TEST C2.watchdog.returned PASS - control returned after " + (new Date().getTime() - t0) + "ms");
console.log("=== SMOKE WATCHDOG END ===");
