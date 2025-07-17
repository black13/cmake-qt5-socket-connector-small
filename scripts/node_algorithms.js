// Node Graph JavaScript Algorithms
// Modern JavaScript (ES6+) examples for NodeGraph processing

// Example 1: Simple node creation and manipulation
function createSimpleGraph() {
    console.log("Creating simple graph with JavaScript");
    
    // Create source node
    const sourceNode = Node.create("Source", -200, 0);
    console.log("Created source node");
    
    // Create processor node
    const processorNode = Node.create("1-to-2", 0, 0);
    console.log("Created processor node");
    
    // Create sink nodes
    const sink1 = Node.create("Sink", 200, -100);
    const sink2 = Node.create("Sink", 200, 100);
    console.log("Created sink nodes");
    
    return {
        source: sourceNode,
        processor: processorNode,
        sinks: [sink1, sink2]
    };
}

// Example 2: Graph traversal algorithm
function traverseGraph() {
    console.log("Traversing graph with JavaScript");
    
    const nodes = Graph.getNodes();
    const edges = Graph.getEdges();
    
    console.log(`Found ${nodes.length} nodes and ${edges.length} edges`);
    
    // Breadth-first traversal
    const visited = new Set();
    const queue = [];
    
    // Start with source nodes
    nodes.forEach(node => {
        if (node.type === "Source") {
            queue.push(node);
        }
    });
    
    while (queue.length > 0) {
        const currentNode = queue.shift();
        
        if (visited.has(currentNode.id)) {
            continue;
        }
        
        visited.add(currentNode.id);
        console.log(`Visiting node: ${currentNode.id} (${currentNode.type})`);
        
        // Add connected nodes to queue
        edges.forEach(edge => {
            if (edge.fromNode === currentNode.id && !visited.has(edge.toNode)) {
                const nextNode = Node.findById(edge.toNode);
                if (nextNode) {
                    queue.push(nextNode);
                }
            }
        });
    }
    
    return Array.from(visited);
}

// Example 3: Force-directed layout algorithm
function forceDirectedLayout(iterations = 100, strength = 0.1) {
    console.log(`Running force-directed layout for ${iterations} iterations`);
    
    const nodes = Graph.getNodes();
    const edges = Graph.getEdges();
    
    // Initialize forces
    const forces = {};
    nodes.forEach(node => {
        forces[node.id] = { x: 0, y: 0 };
    });
    
    for (let i = 0; i < iterations; i++) {
        // Reset forces
        Object.keys(forces).forEach(id => {
            forces[id].x = 0;
            forces[id].y = 0;
        });
        
        // Repulsive forces between all nodes
        nodes.forEach(nodeA => {
            nodes.forEach(nodeB => {
                if (nodeA.id !== nodeB.id) {
                    const dx = nodeA.x - nodeB.x;
                    const dy = nodeA.y - nodeB.y;
                    const distance = Math.sqrt(dx * dx + dy * dy);
                    
                    if (distance > 0) {
                        const force = strength * 1000 / (distance * distance);
                        forces[nodeA.id].x += force * dx / distance;
                        forces[nodeA.id].y += force * dy / distance;
                    }
                }
            });
        });
        
        // Attractive forces for connected nodes
        edges.forEach(edge => {
            const nodeA = Node.findById(edge.fromNode);
            const nodeB = Node.findById(edge.toNode);
            
            if (nodeA && nodeB) {
                const dx = nodeB.x - nodeA.x;
                const dy = nodeB.y - nodeA.y;
                const distance = Math.sqrt(dx * dx + dy * dy);
                
                if (distance > 0) {
                    const force = strength * distance / 100;
                    forces[nodeA.id].x += force * dx / distance;
                    forces[nodeA.id].y += force * dy / distance;
                    forces[nodeB.id].x -= force * dx / distance;
                    forces[nodeB.id].y -= force * dy / distance;
                }
            }
        });
        
        // Apply forces (this would need to be implemented in the Node API)
        nodes.forEach(node => {
            // node.setPosition(node.x + forces[node.id].x, node.y + forces[node.id].y);
            console.log(`Node ${node.id}: force (${forces[node.id].x.toFixed(2)}, ${forces[node.id].y.toFixed(2)})`);
        });
    }
    
    console.log("Force-directed layout complete");
    return forces;
}

// Example 4: Node validation and analysis
function validateGraph() {
    console.log("Validating graph structure");
    
    const nodes = Graph.getNodes();
    const edges = Graph.getEdges();
    const issues = [];
    
    // Check for disconnected nodes
    const connectedNodes = new Set();
    edges.forEach(edge => {
        connectedNodes.add(edge.fromNode);
        connectedNodes.add(edge.toNode);
    });
    
    nodes.forEach(node => {
        if (!connectedNodes.has(node.id) && node.type !== "Source") {
            issues.push(`Node ${node.id} is disconnected`);
        }
    });
    
    // Check for cycles
    const visited = new Set();
    const recursionStack = new Set();
    
    function hasCycle(nodeId) {
        if (recursionStack.has(nodeId)) {
            return true;
        }
        
        if (visited.has(nodeId)) {
            return false;
        }
        
        visited.add(nodeId);
        recursionStack.add(nodeId);
        
        const outgoingEdges = edges.filter(edge => edge.fromNode === nodeId);
        for (const edge of outgoingEdges) {
            if (hasCycle(edge.toNode)) {
                return true;
            }
        }
        
        recursionStack.delete(nodeId);
        return false;
    }
    
    nodes.forEach(node => {
        if (hasCycle(node.id)) {
            issues.push(`Cycle detected involving node ${node.id}`);
        }
    });
    
    if (issues.length === 0) {
        console.log("Graph validation passed");
    } else {
        console.log(`Graph validation found ${issues.length} issues:`);
        issues.forEach(issue => console.log(`  - ${issue}`));
    }
    
    return issues;
}

// Example 5: Data processing pipeline
function processDataPipeline(inputData) {
    console.log("Processing data through graph pipeline");
    
    const nodes = Graph.getNodes();
    const edges = Graph.getEdges();
    
    // Find source nodes
    const sourceNodes = nodes.filter(node => node.type === "Source");
    const nodeOutputs = {};
    
    // Initialize source nodes with input data
    sourceNodes.forEach((node, index) => {
        nodeOutputs[node.id] = inputData[index] || null;
        console.log(`Source node ${node.id}: ${nodeOutputs[node.id]}`);
    });
    
    // Process nodes in topological order
    const processedNodes = new Set();
    const processing = true;
    
    while (processing && processedNodes.size < nodes.length) {
        let foundUnprocessed = false;
        
        nodes.forEach(node => {
            if (processedNodes.has(node.id)) {
                return;
            }
            
            // Check if all input nodes are processed
            const inputEdges = edges.filter(edge => edge.toNode === node.id);
            const allInputsReady = inputEdges.every(edge => processedNodes.has(edge.fromNode));
            
            if (allInputsReady || node.type === "Source") {
                // Process this node
                const inputs = inputEdges.map(edge => nodeOutputs[edge.fromNode]);
                const output = processNode(node, inputs);
                nodeOutputs[node.id] = output;
                processedNodes.add(node.id);
                foundUnprocessed = true;
                
                console.log(`Processed node ${node.id} (${node.type}): ${output}`);
            }
        });
        
        if (!foundUnprocessed) {
            break;
        }
    }
    
    return nodeOutputs;
}

function processNode(node, inputs) {
    // Simple node processing logic
    switch (node.type) {
        case "Source":
            return inputs[0] || "source_data";
        case "Sink":
            return inputs[0] || "sink_data";
        case "1-to-2":
            return inputs[0] ? `split_${inputs[0]}` : "split_data";
        case "2-to-1":
            return inputs.length > 1 ? `merged_${inputs[0]}_${inputs[1]}` : `merged_${inputs[0]}`;
        default:
            return `processed_${inputs[0]}`;
    }
}

// Export functions for use in the application
if (typeof module !== 'undefined' && module.exports) {
    module.exports = {
        createSimpleGraph,
        traverseGraph,
        forceDirectedLayout,
        validateGraph,
        processDataPipeline
    };
}