var nodes = graph.getAllNodes();
for (var i = 0; i < nodes.length; ++i) {
    var id = nodes[i];
    var script = graph.getNodeScript(id);
    if (script && script.trim().length > 0) {
        console.log("Node", id, "script source:\n", script);
    } else {
        console.log("Node", id, "has no script attached");
    }
}
