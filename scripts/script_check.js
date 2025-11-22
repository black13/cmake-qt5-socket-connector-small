var sourceId = graph.createNode("SOURCE", -200, 0);
var transformId = graph.createNode("TRANSFORM", 0, 0);
var sinkId = graph.createNode("SINK", 200, 0);

graph.connectNodes(sourceId, 0, transformId, 0);
graph.connectNodes(transformId, 0, sinkId, 0);

graph.setNodeScript(transformId, `
    var count = node.payloadValue("count") || 0;
    count += 1;
    var result = node.runWork({ task: "loop", iterations: 10000 });
    node.setPayloadValue("count", count);
    return result;
`);

console.log("Running transform script via Graph facade" );
var res = graph.executeNodeScript(transformId, {});
console.log("Result: ", res);
