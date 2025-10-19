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
    output_sockets = []
    input_sockets = []
    for node in nodes:
        for i in range(node['inputs']):
            input_sockets.append((node, i))
        for o in range(node['outputs']):
            socket_index = node['inputs'] + o
            output_sockets.append((node, socket_index))

    random.shuffle(output_sockets)
    random.shuffle(input_sockets)

    edges_created = 0
    pair_count = min(len(output_sockets), len(input_sockets))

    for idx in range(pair_count):
        from_node, from_socket_idx = output_sockets[idx]
        to_node, to_socket_idx = input_sockets[idx]

        if from_node['id'] == to_node['id']:
            swapped = False
            for alt_idx in range(idx + 1, len(input_sockets)):
                candidate_node, candidate_socket_idx = input_sockets[alt_idx]
                if candidate_node['id'] != from_node['id']:
                    input_sockets[idx], input_sockets[alt_idx] = (
                        input_sockets[alt_idx],
                        input_sockets[idx],
                    )
                    to_node, to_socket_idx = candidate_node, candidate_socket_idx
                    swapped = True
                    break
            if not swapped:
                continue

        edge_id = "{" + str(uuid.uuid4()) + "}"
        edge_line = (
            f'  <edge id="{edge_id}" fromNode="{from_node["id"]}" toNode="{to_node["id"]}" '
            f'fromSocketIndex="{from_socket_idx}" toSocketIndex="{to_socket_idx}"/>'
        )
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
    print(f"Created {filename}: {num_nodes} nodes, {edges_created} edges ({file_size:.1f} KB)")

def main():
    """Generate all test files"""
    print("=== NodeGraph Test File Generator ===")
    print("Creating test files following test.xml schema...\n")
    
    # Generate files of increasing sizes
    test_sizes = [
        (6, "tests_tiny.xml"),
        (16, "tests_small.xml"),
        (40, "tests_medium.xml"),
        (80, "tests_large.xml"),
        (160, "tests_stress.xml"),
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
