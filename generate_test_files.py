#!/usr/bin/env python3
"""
Clean Test File Generator for NodeGraph Application
Follows the exact XML schema from test.xml
"""

import uuid
import random
import math
import sys

def generate_graph(num_nodes, filename):
    """Generate XML graph following the exact schema from test.xml"""
    
    print(f"Generating {filename} with {num_nodes} nodes...")
    
    # XML header
    xml_lines = ['<?xml version="1.0" encoding="UTF-8"?>']
    xml_lines.append('<graph version="1.0">')
    
    # Generate nodes in a grid pattern
    grid_size = max(1, int(math.sqrt(num_nodes)))
    spacing = 100
    nodes = []  # Store node info for edge generation
    
    for i in range(num_nodes):
        # Grid position with some randomization
        col = i % grid_size
        row = i // grid_size
        x = col * spacing + random.randint(-20, 20)
        y = row * spacing + random.randint(-20, 20)
        
        # Generate UUID in braces format like test.xml
        node_id = "{" + str(uuid.uuid4()) + "}"
        
        # Random node type and socket counts
        node_type = random.choice(["IN", "OUT"])
        
        if node_type == "IN":
            # IN nodes should have more inputs than outputs (consume data)
            inputs = random.randint(1, 3)
            outputs = random.randint(0, 2)
        else:  # OUT
            # OUT nodes should have more outputs than inputs (produce data)
            inputs = random.randint(0, 2)
            outputs = random.randint(1, 3)
        
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
    
    # Generate edges between nodes
    num_edges = min(num_nodes * 2, num_nodes - 1)  # Reasonable number of edges
    edges_created = 0
    
    for _ in range(num_edges * 3):  # Try multiple times to create edges
        if edges_created >= num_edges:
            break
            
        # Find nodes with compatible sockets
        from_nodes = [n for n in nodes if n['outputs'] > 0]  # has outputs
        to_nodes = [n for n in nodes if n['inputs'] > 0]     # has inputs
        
        if not from_nodes or not to_nodes:
            break
            
        from_node = random.choice(from_nodes)
        to_node = random.choice(to_nodes)
        
        # No self-loops and ensure valid socket connection
        if from_node['id'] != to_node['id']:
            edge_id = "{" + str(uuid.uuid4()) + "}"
            # Use valid socket indices (0-based indexing)
            from_socket_idx = random.randint(0, from_node['outputs'] - 1)
            to_socket_idx = random.randint(0, to_node['inputs'] - 1)
            
            # Create edge XML - exactly like test.xml format
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