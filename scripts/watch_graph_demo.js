// scripts/watch_graph_demo.js
//
// A paced, narrated tour of what the application can do, so every step is
// visible on screen instead of flashing by. Each phase logs what it is about
// to do and then waits, letting the event loop (and therefore animations and
// painting) run via graph.yield(). Total runtime is over a minute.
//
// Run from the repo root:
//   .\run.ps1 --script scripts/watch_graph_demo.js
//
// The window stays open on the finished graph; close it when done.
// Change PACE below to speed up / slow down the whole tour (1.0 = ~31 s safe
// minimum, default 2.2 = ~70 s deliberately watchable).

var PACE = 2.2;

function pause(ms, label) {
    if (label) { console.log(label); }
    graph.yield(Math.round(ms * PACE));
}
function counts() { var s = graph.getGraphStats(); return s.nodeCount + " nodes / " + s.edgeCount + " edges"; }

var start = new Date().getTime();
console.log("WATCH: starting tour (pace x" + PACE + ")");

graph.clearGraph();
pause(600, "-- empty canvas (0 nodes)");

// ── 1. Drop nodes one at a time (they animate in) ──────────────────────────
var s1 = graph.dropNode("SOURCE", -480, -160, 380); pause(700, "-- dropped SOURCE 1");
var s2 = graph.dropNode("SOURCE", -480, 160, 380);  pause(700, "-- dropped SOURCE 2");
var t1 = graph.dropNode("TRANSFORM", -160, -160, 380); pause(700, "-- dropped TRANSFORM 1");
var t2 = graph.dropNode("TRANSFORM", -160, 160, 380);  pause(700, "-- dropped TRANSFORM 2");
var merge = graph.dropNode("MERGE", 160, 0, 380); pause(700, "-- dropped MERGE");
var sink = graph.dropNode("SINK", 480, 0, 380);   pause(900, "-- dropped SINK (" + counts() + ")");

// ── 2. Connect them, one edge at a time ────────────────────────────────────
graph.connectNodes(s1, 0, t1, 0); pause(500, "-- connected SOURCE 1 -> TRANSFORM 1");
graph.connectNodes(s2, 0, t2, 0); pause(500, "-- connected SOURCE 2 -> TRANSFORM 2");
graph.connectNodes(t1, 1, merge, 0); pause(500, "-- connected TRANSFORM 1 -> MERGE");
graph.connectNodes(t2, 1, merge, 1); pause(500, "-- connected TRANSFORM 2 -> MERGE");
graph.connectNodes(merge, 2, sink, 0); pause(800, "-- connected MERGE -> SINK (" + counts() + ")");

// ── 3. Move things around, one at a time ───────────────────────────────────
graph.moveNode(s1, 0, -120); pause(500, "-- nudged SOURCE 1 up");
graph.setNodePosition(t2, -160, 320); pause(500, "-- pushed TRANSFORM 2 down");
graph.setNodePosition(sink, 760, -320); pause(700, "-- dragged SINK far away");

// ── 4. Grid snapping ───────────────────────────────────────────────────────
graph.setSnapToGrid(true); pause(500, "-- snap to grid ON (crosshair)");
graph.snapNodes(); pause(700, "-- snapNodes(): off-grid nodes jump to the grid");
graph.setSnapToGrid(false); pause(400, "-- snap to grid OFF");

// ── 5. Topology alignment ──────────────────────────────────────────────────
graph.setNodePosition(t1, 300, -420); pause(500, "-- scattered TRANSFORM 1");
graph.setNodePosition(merge, -420, 380); pause(500, "-- scattered MERGE");
graph.alignGraph(); pause(1200, "-- alignGraph(): layered layout on the grid (" + counts() + ")");

// ── 6. Undo / redo the alignment ───────────────────────────────────────────
graph.undo(); pause(1000, "-- undo: back to the scattered positions");
graph.redo(); pause(1000, "-- redo: aligned again");

// ── 7. Delete and restore ──────────────────────────────────────────────────
graph.deleteNode(merge); pause(900, "-- deleted MERGE (its edges vanish)");
graph.undo(); pause(900, "-- undo: MERGE and its edges are restored");

// ── 8. Grow a chain, one drop at a time ────────────────────────────────────
console.log("-- growing a chain of transforms, one at a time");
var chain = [];
var prev = "";
for (var i = 0; i < 6; ++i) {
    var id = graph.dropNode("TRANSFORM", 520, -360 + i * 150, 320);
    pause(550);
    if (prev !== "") {
        graph.connectNodes(prev, 1, id, 0);
        pause(300, "-- chain link " + i + " connected");
    } else {
        console.log("-- chain node 0 dropped");
    }
    chain.push(id);
    prev = id;
}
pause(600, "-- chain complete (" + counts() + ")");

// ── 9. Lay the chain out in a row, watching each move ──────────────────────
for (var j = 0; j < chain.length; ++j) {
    graph.setNodePosition(chain[j], 560 + j * 200, 460);
    pause(320, "-- moved chain node " + j);
}

// ── 10. Snap + align + center the whole thing ──────────────────────────────
graph.setSnapToGrid(true);
graph.snapNodes(); pause(500, "-- everything snapped to the grid");
graph.alignGraph(); pause(1100, "-- aligned the full graph (" + counts() + ")");
view.centerOnGraph(); pause(900, "-- view centered; snap OFF");
graph.setSnapToGrid(false);

// ── 11. Save, clear, reload ────────────────────────────────────────────────
graph.saveToFile("logs/watch_demo.xml"); pause(600, "-- saved to logs/watch_demo.xml");
graph.clearGraph(); pause(1000, "-- cleared the canvas (0 nodes)");
graph.loadFromFile("logs/watch_demo.xml"); pause(1200, "-- reloaded the saved graph (" + counts() + ")");
view.centerOnGraph(); pause(600, "-- centered on the reloaded graph");

var elapsed = Math.round((new Date().getTime() - start) / 1000);
console.log("WATCH: tour complete in " + elapsed + "s - window stays open, close it when done");
