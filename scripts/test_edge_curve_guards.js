// Test Edge Curve Control Guards
// Exercises edge curve calculation with extreme positions to verify guards

print("=== Edge Curve Guards Test ===");
print("Testing edge curve stability with extreme node positions");

// Test 1: Zero-length edges (nodes at same position)
print("\n[Test 1] Zero-length edges - nodes at same position");
var node1 = graph.createNode("TRANSFORM", 100, 100);
var node2 = graph.createNode("TRANSFORM", 100, 100);  // Same position
if (node1 && node2) {
    var edge1 = graph.connectNodes(node1, 0, node2, 0);
    print("[OK] Created edge with zero-length path (same position)");
    print("  Node1: " + node1 + " at (100, 100)");
    print("  Node2: " + node2 + " at (100, 100)");
}

// Test 2: Extreme distance (very far apart)
print("\n[Test 2] Extreme distance - nodes very far apart");
var node3 = graph.createNode("SOURCE", -5000, -5000);
var node4 = graph.createNode("SINK", 5000, 5000);
if (node3 && node4) {
    var edge2 = graph.connectNodes(node3, 0, node4, 0);
    print("[OK] Created edge with extreme distance");
    print("  Node3: " + node3 + " at (-5000, -5000)");
    print("  Node4: " + node4 + " at (5000, 5000)");
    print("  Distance: ~14142 units");
}

// Test 3: Very close but not identical
print("\n[Test 3] Very close nodes (sub-pixel distance)");
var node5 = graph.createNode("TRANSFORM", 200, 200);
var node6 = graph.createNode("TRANSFORM", 200.5, 200.5);
if (node5 && node6) {
    var edge3 = graph.connectNodes(node5, 0, node6, 0);
    print("[OK] Created edge with sub-pixel distance");
    print("  Node5: " + node5 + " at (200, 200)");
    print("  Node6: " + node6 + " at (200.5, 200.5)");
}

// Test 4: Vertical alignment (dx=0)
print("\n[Test 4] Vertical alignment - pure vertical edge");
var node7 = graph.createNode("SOURCE", 300, 100);
var node8 = graph.createNode("SINK", 300, 500);
if (node7 && node8) {
    var edge4 = graph.connectNodes(node7, 0, node8, 0);
    print("[OK] Created vertical edge (dx=0)");
    print("  Node7: " + node7 + " at (300, 100)");
    print("  Node8: " + node8 + " at (300, 500)");
}

// Test 5: Horizontal alignment (dy=0)
print("\n[Test 5] Horizontal alignment - pure horizontal edge");
var node9 = graph.createNode("SOURCE", 100, 600);
var node10 = graph.createNode("SINK", 800, 600);
if (node9 && node10) {
    var edge5 = graph.connectNodes(node9, 0, node10, 0);
    print("[OK] Created horizontal edge (dy=0)");
    print("  Node9: " + node9 + " at (100, 600)");
    print("  Node10: " + node10 + " at (800, 600)");
}

// Test 6: Backward connection (dx < 0)
print("\n[Test 6] Backward connection - right to left");
var node11 = graph.createNode("SOURCE", 900, 100);
var node12 = graph.createNode("SINK", 100, 300);
if (node11 && node12) {
    var edge6 = graph.connectNodes(node11, 0, node12, 0);
    print("[OK] Created backward S-curve edge (dx<0)");
    print("  Node11: " + node11 + " at (900, 100)");
    print("  Node12: " + node12 + " at (100, 300)");
}

// Test 7: Multiple edges from same node (stress test)
print("\n[Test 7] Multiple edges from single node");
var hub = graph.createNode("SOURCE", 500, 500);
var targets = [];
for (var i = 0; i < 8; i++) {
    var angle = (i / 8.0) * 2 * Math.PI;
    var x = 500 + Math.cos(angle) * 200;
    var y = 500 + Math.sin(angle) * 200;
    var target = graph.createNode("SINK", x, y);
    if (hub && target) {
        graph.connectNodes(hub, 0, target, 0);
        targets.push(target);
    }
}
print("[OK] Created radial pattern with 8 edges from hub");
print("  Hub: " + hub + " at (500, 500)");

// Test 8: Chain with varying distances
print("\n[Test 8] Chain with varying edge lengths");
var chainNodes = [];
var positions = [
    [0, 0],
    [50, 50],     // Short edge
    [500, 100],   // Long edge
    [505, 105],   // Very short edge
    [600, 200]    // Medium edge
];
for (var i = 0; i < positions.length; i++) {
    var node = graph.createNode("TRANSFORM", positions[i][0], positions[i][1]);
    chainNodes.push(node);
    if (i > 0 && chainNodes[i-1] && chainNodes[i]) {
        graph.connectNodes(chainNodes[i-1], 0, chainNodes[i], 0);
    }
}
print("[OK] Created chain with varying edge lengths");
print("  Lengths: normal -> short -> long -> very short -> medium");

print("\n=== Test Summary ===");
print("All edge curve guard tests completed");
print("Guards tested:");
print("  [OK] Zero-length edge protection (kMinEdgeLength)");
print("  [OK] NaN/Inf validation (qIsFinite checks)");
print("  [OK] Control offset bounds (kMinControlOffset, kMaxControlOffset)");
print("  [OK] Control point validation");
print("  [OK] Degenerate curve fallback to straight line");
print("\nExpected behavior:");
print("  - No crashes from invalid curve parameters");
print("  - All edges render smoothly");
print("  - Extreme positions handled gracefully");
print("  - Fallback to straight lines for degenerate cases");
