const sourceId = graph.createNode("SOURCE", -150, 0);
const scriptedId = graph.createNode("TRANSFORM", 0, 0);
const sinkId = graph.createNode("SINK", 200, 0);

graph.setNodeScript(scriptedId, `
    var count = node.payloadValue("count") || 0;
    count += 1;
    node.setPayloadValue("count", count);
    node.setLabel("Runs: " + count);
    node.log("Script executed " + count + " time(s)");
    return count;
`);

graph.connectNodes(sourceId, 0, scriptedId, 0);
graph.connectNodes(scriptedId, 0, sinkId, 0);

console.log("Created scriptable node:", scriptedId);

for (var i = 0; i < 3; ++i) {
    const result = graph.executeNodeScript(scriptedId, {});
    console.log("Script execution result #" + (i + 1) + ": " + result);
}

graph.saveToFile("scripted_node_demo.xml");
console.log("Saved scripted node demo to scripted_node_demo.xml");
