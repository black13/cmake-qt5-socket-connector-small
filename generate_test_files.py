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

def generate_crash_test():
    """Generate a specific test designed to trigger XML serialization crashes"""
    print("Generating test_crash_reproduction.xml to trigger XML serialization issues...")
    
    xml_lines = ['<?xml version="1.0" encoding="UTF-8"?>']
    xml_lines.append('<graph version="1.0">')
    
    # Create nodes with many sockets and complex connections
    nodes = []
    spacing = 100
    
    # Create a mix of nodes with varying socket counts
    for i in range(50):
        x = (i % 10) * spacing + 50
        y = (i // 10) * spacing + 50
        node_id = "{" + str(uuid.uuid4()) + "}"
        
        # Create nodes with higher socket counts to stress the system
        if i % 3 == 0:  # Every 3rd node: Heavy input node
            inputs = random.randint(3, 5)  # 3-5 inputs
            outputs = random.randint(1, 2)  # 1-2 outputs
            node_type = "PROCESSOR"
        elif i % 3 == 1:  # Every 3rd node: Heavy output node  
            inputs = random.randint(1, 2)   # 1-2 inputs
            outputs = random.randint(3, 5)  # 3-5 outputs
            node_type = "PROCESSOR"
        else:  # Regular nodes
            inputs = random.randint(0, 2)
            outputs = random.randint(0, 2)
            node_type = random.choice(["IN", "OUT", "PROCESSOR"])
        
        nodes.append({
            'id': node_id,
            'type': node_type,
            'inputs': inputs,
            'outputs': outputs
        })
        
        node_line = f'  <node id="{node_id}" x="{x}" y="{y}" type="{node_type}" inputs="{inputs}" outputs="{outputs}"/>'
        xml_lines.append(node_line)
    
    # Create many edges to trigger autosave and serialization stress
    edges_created = 0
    for _ in range(200):  # Try to create many edges
        from_nodes = [n for n in nodes if n['outputs'] > 0]
        to_nodes = [n for n in nodes if n['inputs'] > 0]
        
        if not from_nodes or not to_nodes:
            break
            
        from_node = random.choice(from_nodes)
        to_node = random.choice(to_nodes)
        
        if from_node['id'] != to_node['id']:
            edge_id = "{" + str(uuid.uuid4()) + "}"
            
            output_start_idx = from_node['inputs']
            from_socket_idx = random.randint(output_start_idx, output_start_idx + from_node['outputs'] - 1)
            to_socket_idx = random.randint(0, to_node['inputs'] - 1)
            
            edge_line = f'  <edge id="{edge_id}" fromNode="{from_node["id"]}" toNode="{to_node["id"]}" fromSocketIndex="{from_socket_idx}" toSocketIndex="{to_socket_idx}"/>'
            xml_lines.append(edge_line)
            edges_created += 1
    
    xml_lines.append('</graph>')
    
    # Write crash test file
    with open("test_crash_reproduction.xml", 'w', encoding='utf-8') as f:
        f.write('\n'.join(xml_lines))
        f.write('\n')
    
    import os
    file_size = os.path.getsize("test_crash_reproduction.xml") / 1024
    print(f"✓ Created test_crash_reproduction.xml: 50 nodes, {edges_created} edges ({file_size:.1f} KB)")
    print("  This file is designed to trigger XML serialization and autosave stress!")

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
        (100, "test_stress.xml"),     # Stress: 10x10 grid for stress testing
        (500, "test_extreme.xml"),    # Extreme: Heavy stress test for crash reproduction
        (1000, "test_brutal.xml")     # Brutal: Maximum stress test
    ]
    
    for num_nodes, filename in test_sizes:
        generate_graph(num_nodes, filename)
    
    # Generate the special crash reproduction test
    print()
    generate_crash_test()
    
    print(f"\n✅ Generated {len(test_sizes)} test files + 1 crash test")
    print("\n📖 Usage:")
    print("   ./NodeGraph                    # Default nodes + autosave")
    print("   ./NodeGraph test_simple.xml    # Load 2-node simple test") 
    print("   ./NodeGraph test_small.xml     # Load 4-node small test")
    print("   ./NodeGraph test_medium.xml    # Load 9-node medium test")
    print("   ./NodeGraph test_large.xml     # Load 16-node large test")
    print("   ./NodeGraph test_stress.xml    # Load 100-node stress test")
    print("   ./NodeGraph test_extreme.xml   # Load 500-node extreme test")
    print("   ./NodeGraph test_brutal.xml    # Load 1000-node brutal test")
    print("   ./NodeGraph test_crash_reproduction.xml  # Load crash test")
    print("\n⚠️  WARNING: Extreme and Brutal tests may cause crashes!")
    print("   Use these to reproduce XML serialization and memory issues.")
    print("   test_crash_reproduction.xml specifically targets autosave crashes!")

if __name__ == "__main__":
    main()