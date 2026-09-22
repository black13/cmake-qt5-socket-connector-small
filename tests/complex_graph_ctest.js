// tests/complex_graph_ctest.js
//
// CTest wrapper for scripts/complex_graph_demo.js. The demo deliberately
// leaves the window open for interactive use, so this wrapper (run with
// --script) executes it and exits with the demo's failure count.
//
// The demo declares its counters with top-level var, which QJSEngine places
// on the global object, so `fails` is readable here after evalFile().

var executed = graph.evalFile("scripts/complex_graph_demo.js");
var failed = (typeof fails === "number") ? fails : 1;
console.log("COMPLEX GRAPH CTEST: fails=" + failed);
graph.quit(failed === 0 ? 0 : 1);

// Keep lint-style tools quiet about the intentionally-unused eval result.
void executed;
