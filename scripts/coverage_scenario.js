// coverage_scenario.js - drives Graph facade operations to boost coverage

function log(message) {
    console.log("[coverage] " + message);
}

function safeRun(label, fn) {
    try {
        fn();
    } catch (err) {
        console.log("[coverage][error] " + label + ": " + err);
    }
}

log("Scenario start");

graph.beginBatch();
var layout = [
    { type: "SOURCE", x: -350, y: -120 },
    { type: "TRANSFORM", x: -120, y: -90 },
    { type: "TRANSFORM", x: 80, y: -30 },
    { type: "MERGE", x: 260, y: 60 },
    { type: "SINK", x: 420, y: 90 }
];
var nodeIds = [];
layout.forEach(function(desc, index) {
    safeRun("createNode(" + desc.type + ")", function() {
        var id = graph.createNode(desc.type, desc.x, desc.y);
        nodeIds.push(id);
        log("Created node " + desc.type + " -> " + id);
    });
});
graph.endBatch();

var edgeIds = [];
function connect(fromIndex, toIndex, fromSocket, toSocket) {
    safeRun("connect" + fromIndex + "->" + toIndex, function() {
        var id = graph.connectNodes(nodeIds[fromIndex], fromSocket || 0, nodeIds[toIndex], toSocket || 0);
        edgeIds.push(id);
        log("Connected nodes " + fromIndex + "->" + toIndex + " edge=" + id);
    });
}

connect(0, 1);
connect(1, 2);
connect(2, 3);
connect(0, 3, 0, 1);
connect(3, 4);

safeRun("setNodePayload", function() {
    graph.setNodePayload(nodeIds[1], { role: "processor", idx: 1 });
    graph.setNodePayload(nodeIds[3], { role: "merge", note: "coverage" });
});

safeRun("executeNodeScript", function() {
    graph.executeNodeScript(nodeIds[1], { reason: "coverage" });
});

safeRun("moveNode", function() {
    graph.moveNode(nodeIds[2], 35, -25);
});

safeRun("setNodePosition", function() {
    graph.setNodePosition(nodeIds[4], 460, 120);
});

safeRun("getNodeData", function() {
    var info = graph.getNodeData(nodeIds[2]);
    log("Node data: " + JSON.stringify(info));
});

safeRun("getEdgeData", function() {
    if (edgeIds.length > 0) {
        var info = graph.getEdgeData(edgeIds[0]);
        log("Edge data: " + JSON.stringify(info));
    }
});

safeRun("synthetic work", function() {
    var work = graph.runSyntheticWork({ task: "loop", iterations: 20000 });
    log("Synthetic work result: " + JSON.stringify(work));
});

safeRun("stats", function() {
    var stats = graph.getGraphStats();
    log("Graph stats: " + JSON.stringify(stats));
});

safeRun("save stage", function() {
    graph.saveToFile("codecoverage_stage1.xml");
});

safeRun("load stage", function() {
    graph.clearGraph();
    graph.loadFromFile("codecoverage_stage1.xml");
    log("Reloaded stage1 XML");
});

safeRun("delete after reload", function() {
    var nodes = graph.getAllNodes();
    if (nodes.length > 0) {
        graph.deleteNode(nodes[0]);
        log("Deleted node after reload: " + nodes[0]);
    }
    var edges = graph.getAllEdges();
    if (edges.length > 0) {
        graph.deleteEdge(edges[0]);
        log("Deleted edge after reload: " + edges[0]);
    }
});

safeRun("load complete script", function() {
    graph.clearGraph();
    graph.loadFromFile("complete_scripted.xml");
    log("Loaded complete_scripted.xml for coverage");
});

safeRun("final save", function() {
    graph.saveToFile("codecoverage.xml");
});

safeRun("final clear", function() {
    graph.clearGraph();
});

safeRun("quit", function() {
    graph.quitApplication();
});

log("Scenario complete");
