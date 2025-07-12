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
    """Generate XML graph following the exact schema with one edge per socket"""
    
    print(f"Generating {filename} with {num_nodes} nodes using {layout_type} layout...")
    
    # XML header
    xml_lines = ['<?xml version="1.0" encoding="UTF-8"?>']
    xml_lines.append('<graph version="1.0">')
    
    # Generate nodes with clean layout
    nodes = []  # Store node info for edge generation
    spacing = 150  # Increased spacing for better visibility
    grid_size = math.ceil(math.sqrt(num_nodes))  # Square grid layout
    
    for i in range(num_nodes):
        # Clean grid layout with proper spacing
        col = i % grid_size
        row = i // grid_size
        x = col * spacing + 50  # Start offset
        y = row * spacing + 50  # Start offset
        
        # Generate UUID in braces format like test.xml
        node_id = "{" + str(uuid.uuid4()) + "}"
        
        # Balanced node types and socket counts for clean connections
        node_type = random.choice(["IN", "OUT", "PROCESSOR"])
        
        if node_type == "IN":
            # IN nodes: minimal inputs, focus on outputs (data sources)
            inputs = 0
            outputs = random.randint(1, 2)  # 1-2 outputs
        elif node_type == "OUT":
            # OUT nodes: focus on inputs, minimal outputs (data sinks)
            inputs = random.randint(1, 2)   # 1-2 inputs
            outputs = 0
        else:  # PROCESSOR
            # PROCESSOR nodes: balanced input/output (data transformers)
            inputs = random.randint(1, 2)   # 1-2 inputs
            outputs = random.randint(1, 2)  # 1-2 outputs
        
        # Store node info for edge generation with socket tracking
        nodes.append({
            'id': node_id,
            'type': node_type,
            'inputs': inputs,
            'outputs': outputs,
            'x': x,
            'y': y,
            'used_input_sockets': [],   # Track which input sockets are used
            'used_output_sockets': []   # Track which output sockets are used
        })
        
        # Create node XML - exactly like test.xml format
        node_line = f'  <node id="{node_id}" x="{x}" y="{y}" type="{node_type}" inputs="{inputs}" outputs="{outputs}"/>'
        xml_lines.append(node_line)
    
    # Generate edges ensuring one edge per socket (clean connections)
    edges_created = 0
    
    # Create a pool of available output sockets (sources)
    available_outputs = []
    for node in nodes:
        if node['outputs'] > 0:
            output_start_idx = node['inputs']  # outputs start after inputs
            for i in range(node['outputs']):
                available_outputs.append({
                    'node': node,
                    'socket_idx': output_start_idx + i
                })
    
    # Create a pool of available input sockets (targets)
    available_inputs = []
    for node in nodes:
        if node['inputs'] > 0:
            for i in range(node['inputs']):
                available_inputs.append({
                    'node': node,
                    'socket_idx': i
                })
    
    # Shuffle for random connections
    random.shuffle(available_outputs)
    random.shuffle(available_inputs)
    
    # Connect outputs to inputs (one edge per socket)
    max_connections = min(len(available_outputs), len(available_inputs))
    
    for i in range(max_connections):
        output_socket = available_outputs[i]
        input_socket = available_inputs[i]
        
        # Prevent self-loops
        if output_socket['node']['id'] != input_socket['node']['id']:
            edge_id = "{" + str(uuid.uuid4()) + "}"
            
            # Create edge XML
            edge_line = f'  <edge id="{edge_id}" fromNode="{output_socket["node"]["id"]}" toNode="{input_socket["node"]["id"]}" fromSocketIndex="{output_socket["socket_idx"]}" toSocketIndex="{input_socket["socket_idx"]}"/>'
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
    
    # Generate clean test files focused on ghost edge testing
    test_sizes = [
        (2, "test_simple.xml"),       # Simple: 2 nodes for basic connection testing
        (4, "test_small.xml"),        # Small: 4 nodes for ghost edge testing
        (9, "test_medium.xml"),       # Medium: 3x3 grid for layout testing
        (16, "test_large.xml"),       # Large: 4x4 grid for performance testing
        (25, "test_stress.xml")       # Stress: 5x5 grid for stress testing
    ]
    
    for num_nodes, filename in test_sizes:
        generate_graph(num_nodes, filename)
    
    print(f"\n✅ Generated {len(test_sizes)} test files")
    print("\n📖 Usage:")
    print("   ./NodeGraph                    # Default nodes + autosave")
    print("   ./NodeGraph test_simple.xml    # Load 2-node simple test") 
    print("   ./NodeGraph test_small.xml     # Load 4-node small test")
    print("   ./NodeGraph test_medium.xml    # Load 9-node medium test")
    print("   ./NodeGraph test_large.xml     # Load 16-node large test")
    print("   ./NodeGraph test_stress.xml    # Load 25-node stress test")

if __name__ == "__main__":
    main()