#!/usr/bin/env python3
"""
NodeGraph XML Generator
Creates XML files compatible with the NodeGraph application's XML-first architecture.

Usage:
    python generate_graph.py --help
    python generate_graph.py simple
    python generate_graph.py complex
    python generate_graph.py custom --nodes 5 --edges 8
"""

import argparse
import xml.etree.ElementTree as ET
import uuid
import random
import math

class NodeGraphGenerator:
    def __init__(self):
        self.nodes = []
        self.edges = []
        
    def create_node(self, node_type, x, y, inputs=1, outputs=1):
        """Create a node with specified socket configuration"""
        node = {
            'id': str(uuid.uuid4()),
            'type': node_type,
            'x': x,
            'y': y,
            'inputs': inputs,
            'outputs': outputs
        }
        self.nodes.append(node)
        return node
        
    def create_edge(self, from_node, from_socket_index, to_node, to_socket_index):
        """Create an edge between two node sockets using node+index references"""
        edge = {
            'id': str(uuid.uuid4()),
            'fromNode': from_node['id'],
            'toNode': to_node['id'],
            'fromSocketIndex': from_socket_index,
            'toSocketIndex': to_socket_index
        }
        self.edges.append(edge)
        return edge
        
    def _generate_socket_id(self, node_id, socket_index):
        """Generate deterministic socket UUID based on node ID and socket index"""
        # Create a namespace UUID from node ID and socket index
        namespace = uuid.UUID(node_id)
        socket_string = f"{node_id}-socket-{socket_index}"
        return str(uuid.uuid5(namespace, socket_string))
        
    def generate_simple_chain(self):
        """Generate nodes with specific connection requirements"""
        print("Generating simple connection test...")
        
        # Pure OUT node: 0 inputs, exactly 1 output (makes one output connection)
        out_node = self.create_node("OUT", 100, 100, inputs=0, outputs=1)
        
        # Middle node: at least 1 input, at least 1 output  
        middle_node = self.create_node("OUT", 300, 100, inputs=1, outputs=1)
        
        # Pure IN node: exactly 1 input, 0 outputs (makes one input connection)
        in_node = self.create_node("IN", 500, 100, inputs=1, outputs=0)
        
        # Connect: one output connection -> one input connection
        # Socket indices: inputs first (0 to inputs-1), then outputs (inputs to inputs+outputs-1)
        self.create_edge(out_node, 0, middle_node, 0)   # OUT output 0 -> middle input 0
        self.create_edge(middle_node, 1, in_node, 0)    # middle output (index 1) -> IN input 0
        
    def generate_complex_network(self):
        """Generate a simple network with OUT and IN nodes"""
        print("Generating simple OUT->IN network...")
        
        # OUT nodes
        out1 = self.create_node("OUT", 100, 50, inputs=0, outputs=2)
        out2 = self.create_node("OUT", 100, 150, inputs=0, outputs=1)
        out3 = self.create_node("OUT", 300, 100, inputs=2, outputs=2)
        
        # IN nodes  
        in1 = self.create_node("IN", 500, 75, inputs=2, outputs=0)
        in2 = self.create_node("IN", 500, 175, inputs=1, outputs=0)
        
        # Connect OUT outputs to OUT/IN inputs
        # Socket indices: inputs first (0 to inputs-1), then outputs (inputs to inputs+outputs-1)
        # out1: inputs=0, outputs=2 -> output indices are 0,1
        # out2: inputs=0, outputs=1 -> output index is 0  
        # out3: inputs=2, outputs=2 -> input indices are 0,1 and output indices are 2,3
        # in1: inputs=2, outputs=0 -> input indices are 0,1
        # in2: inputs=1, outputs=0 -> input index is 0
        self.create_edge(out1, 0, out3, 0)    # out1 output 0 -> out3 input 0
        self.create_edge(out2, 0, out3, 1)    # out2 output 0 -> out3 input 1
        self.create_edge(out1, 1, in1, 0)     # out1 output 1 -> in1 input 0
        self.create_edge(out3, 2, in1, 1)     # out3 output 0 (index 2) -> in1 input 1
        self.create_edge(out3, 3, in2, 0)     # out3 output 1 (index 3) -> in2 input 0
        
    def generate_custom_graph(self, node_count=5, edge_count=6):
        """Generate a custom graph with specified node and edge counts"""
        print(f"Generating custom graph with {node_count} nodes and {edge_count} edges...")
        
        # Create nodes in a grid layout
        cols = int(math.ceil(math.sqrt(node_count)))
        for i in range(node_count):
            row = i // cols
            col = i % cols
            x = 100 + col * 200
            y = 100 + row * 150
            
            # Determine node type and socket counts
            if i == 0:
                # First node: OUT only (0 inputs, 1-2 outputs)
                node_type = "OUT"
                inputs = 0
                outputs = random.randint(1, 2)
            elif i == node_count - 1:
                # Last node: IN only (1-2 inputs, 0 outputs)
                node_type = "IN"
                inputs = random.randint(1, 2)
                outputs = 0
            else:
                # Middle nodes: mostly OUT (can have inputs and outputs)
                node_type = "OUT"
                inputs = random.randint(1, 2)
                outputs = random.randint(1, 2)
                    
            self.create_node(node_type, x, y, inputs, outputs)
            
        # Create random edges
        edge_attempts = 0
        max_attempts = edge_count * 10
        
        while len(self.edges) < edge_count and edge_attempts < max_attempts:
            from_node = random.choice(self.nodes)
            to_node = random.choice(self.nodes)
            
            # Don't connect node to itself
            if from_node == to_node:
                edge_attempts += 1
                continue
                
            # Check if from_node has outputs and to_node has inputs
            if from_node['outputs'] == 0 or to_node['inputs'] == 0:
                edge_attempts += 1
                continue
                
            # Pick random socket indexes
            from_socket = from_node['inputs'] + random.randint(0, from_node['outputs'] - 1)
            to_socket = random.randint(0, to_node['inputs'] - 1)
            
            # Check if this connection already exists
            existing = any(e['fromNode'] == from_node['id'] and 
                          e['toNode'] == to_node['id'] and
                          e['fromSocketIndex'] == from_socket and
                          e['toSocketIndex'] == to_socket 
                          for e in self.edges)
                          
            if not existing:
                self.create_edge(from_node, from_socket, to_node, to_socket)
                
            edge_attempts += 1
            
    def to_xml(self):
        """Convert the graph to XML format"""
        # Create root element
        root = ET.Element("graph")
        root.set("version", "1.0")
        root.set("xmlns", "http://nodegraph.org/schema")
        
        # Add nodes
        for node in self.nodes:
            node_elem = ET.SubElement(root, "node")
            node_elem.set("id", node['id'])
            node_elem.set("type", node['type'])
            node_elem.set("x", str(node['x']))
            node_elem.set("y", str(node['y']))
            node_elem.set("inputs", str(node['inputs']))
            node_elem.set("outputs", str(node['outputs']))
            
        # Add edges
        for edge in self.edges:
            edge_elem = ET.SubElement(root, "edge")
            edge_elem.set("id", edge['id'])
            edge_elem.set("fromNode", edge['fromNode'])
            edge_elem.set("toNode", edge['toNode'])
            edge_elem.set("fromSocketIndex", str(edge['fromSocketIndex']))
            edge_elem.set("toSocketIndex", str(edge['toSocketIndex']))
            
        return root
        
    def save_xml(self, filename):
        """Save the graph as an XML file"""
        root = self.to_xml()
        
        # Pretty print the XML
        ET.indent(root, space="  ", level=0)
        tree = ET.ElementTree(root)
        
        tree.write(filename, encoding='utf-8', xml_declaration=True)
        print(f"Graph saved to: {filename}")
        
        # Print summary
        print(f"Created {len(self.nodes)} nodes and {len(self.edges)} edges")
        for node in self.nodes:
            print(f"  Node {node['type']}: {node['inputs']} inputs, {node['outputs']} outputs")

def main():
    parser = argparse.ArgumentParser(description='Generate NodeGraph XML files')
    parser.add_argument('type', choices=['simple', 'complex', 'custom'], 
                       help='Type of graph to generate')
    parser.add_argument('--nodes', type=int, default=5,
                       help='Number of nodes for custom graph (default: 5)')
    parser.add_argument('--edges', type=int, default=6,
                       help='Number of edges for custom graph (default: 6)')
    parser.add_argument('--output', '-o', default=None,
                       help='Output filename (default: auto-generated)')
    
    args = parser.parse_args()
    
    generator = NodeGraphGenerator()
    
    # Generate the specified graph type
    if args.type == 'simple':
        generator.generate_simple_chain()
        default_filename = 'simple_graph.xml'
    elif args.type == 'complex':
        generator.generate_complex_network()
        default_filename = 'complex_graph.xml'
    elif args.type == 'custom':
        generator.generate_custom_graph(args.nodes, args.edges)
        default_filename = f'custom_graph_{args.nodes}n_{args.edges}e.xml'
    
    # Use specified filename or default
    filename = args.output or default_filename
    
    # Save the graph
    generator.save_xml(filename)
    
    print(f"\nTo test with NodeGraph application:")
    print(f"./build_Debug/Debug/NodeGraph.exe --load {filename}")
    print(f"# or")
    print(f"./build_Debug/Debug/NodeGraph.exe {filename}")

if __name__ == '__main__':
    main()