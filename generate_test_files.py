#!/usr/bin/env python3
"""
Clean Test File Generator for NodeGraph Application
Follows the exact XML schema from test.xml
"""

import uuid
import random
import math
import sys

def generate_graph(num_nodes, filename, layout_type="grid"):
    """Generate XML graph following the exact schema from test.xml"""
    
    print(f"Generating {filename} with {num_nodes} nodes using {layout_type} layout...")
    
    # XML header
    xml_lines = ['<?xml version="1.0" encoding="UTF-8"?>']
    xml_lines.append('<graph version="1.0">')
    
    # Generate nodes with specified layout
    nodes = []  # Store node info for edge generation
    spacing = 100
    grid_size = max(3, int(math.sqrt(num_nodes)) + 1)  # Calculate grid size based on number of nodes
    
    for i in range(num_nodes):
        # Grid position with some randomization
        col = i % grid_size
        row = i // grid_size
        x = col * spacing + random.randint(-20, 20)
        y = row * spacing + random.randint(-20, 20)
        
        # Generate UUID in braces format like test.xml
        node_id = "{" + str(uuid.uuid4()) + "}"
        
        # Use ONLY node types supported by the NodeTypeTemplates system
        node_types = [
            ("SOURCE", 0, 1),    # Source: 0 inputs, 1 output
            ("SINK", 1, 0),      # Sink: 1 input, 0 outputs  
            ("TRANSFORM", 1, 1), # Transform: 1 input, 1 output
            ("MERGE", 2, 1),     # Merge: 2 inputs, 1 output
            ("SPLIT", 1, 2),     # Split: 1 input, 2 outputs
        ]
        
        # Choose a random node type with proper socket configuration
        node_type, inputs, outputs = random.choice(node_types)
        
        # Store node info for edge generation
        nodes.append({
            'id': node_id,
            'type': node_type,
            'inputs': inputs,
            'outputs': outputs
        })
        
        # Create node XML - exactly like test.xml format
        node_line = f'  <node id="{node_id}" x="{x}" y="{y}" type="{node_type}" inputs="{inputs}" outputs="{outputs}"/>'
        xml_lines.append(node_line)
    
    # Generate edges between nodes with socket usage tracking
    num_edges = min(num_nodes * 2, num_nodes - 1)  # Reasonable number of edges
    edges_created = 0
    
    # Track used sockets to prevent duplicates: "nodeId:socketIndex" -> True
    used_sockets = set()
    
    for _ in range(num_edges * 10):  # More attempts to find valid connections
        if edges_created >= num_edges:
            break
            
        # Find nodes with compatible sockets
        from_nodes = [n for n in nodes if n['outputs'] > 0]  # has outputs
        to_nodes = [n for n in nodes if n['inputs'] > 0]     # has inputs
        
        if not from_nodes or not to_nodes:
            break
            
        from_node = random.choice(from_nodes)
        to_node = random.choice(to_nodes)
        
        # No self-loops
        if from_node['id'] == to_node['id']:
            continue
            
        # SOCKET INDEXING: follows C++ Node::createSocketsFromXml()
        # Input sockets: indices 0, 1, 2, ..., (inputCount-1) 
        # Output sockets: indices inputCount, inputCount+1, ..., (inputCount+outputCount-1)
        
        # fromSocket must be an OUTPUT socket (from fromNode)
        output_start_idx = from_node['inputs']  # outputs start after inputs
        from_socket_idx = random.randint(output_start_idx, output_start_idx + from_node['outputs'] - 1)
        
        # toSocket must be an INPUT socket (from toNode) 
        to_socket_idx = random.randint(0, to_node['inputs'] - 1)
        
        # Create socket keys to check for duplicates
        from_socket_key = f"{from_node['id']}:{from_socket_idx}"
        to_socket_key = f"{to_node['id']}:{to_socket_idx}"
        
        # Check if either socket is already used
        if from_socket_key in used_sockets or to_socket_key in used_sockets:
            continue  # Skip this combination, try another
        
        # Mark sockets as used
        used_sockets.add(from_socket_key)
        used_sockets.add(to_socket_key)
        
        # Create valid edge
        edge_id = "{" + str(uuid.uuid4()) + "}"
        edge_line = f'  <edge id="{edge_id}" fromNode="{from_node["id"]}" toNode="{to_node["id"]}" fromSocketIndex="{from_socket_idx}" toSocketIndex="{to_socket_idx}"/>'
        xml_lines.append(edge_line)
        edges_created += 1
    
    xml_lines.append('</graph>')
    
    print(f"  Generated {edges_created} edges")
    
    # Write file
    with open(filename, 'w', encoding='utf-8') as f:
        f.write('\n'.join(xml_lines))
        f.write('\n')  # Final newline
    
    # Report file size
    import os
    file_size = os.path.getsize(filename) / 1024
    print(f"✓ Created {filename}: {num_nodes} nodes, {edges_created} edges ({file_size:.1f} KB)")

def main():
    """Generate all test files"""
    print("=== NodeGraph Test File Generator ===")
    print("Creating test files following test.xml schema...\n")
    
    # Generate files of increasing sizes
    test_sizes = [
        (10, "tests_tiny.xml"),     # Very small for quick testing
        (100, "tests_small.xml"),   # Small test
        (500, "tests_medium.xml"),  # Medium test  
        (1000, "tests_large.xml"),  # Large test
        (5000, "tests_stress.xml")  # Stress test
    ]
    
    for num_nodes, filename in test_sizes:
        generate_graph(num_nodes, filename)
    
    print(f"\n✅ Generated {len(test_sizes)} test files")
    print("\n📖 Usage:")
    print("   ./NodeGraph                    # Default nodes + autosave")
    print("   ./NodeGraph tests_tiny.xml     # Load tiny test file") 
    print("   ./NodeGraph tests_small.xml    # Load small test file")
    print("   ./NodeGraph tests_medium.xml   # Load medium test file")
    print("   ./NodeGraph tests_large.xml    # Load large test file")
    print("   ./NodeGraph tests_stress.xml   # Load stress test file")

if __name__ == "__main__":
    main()