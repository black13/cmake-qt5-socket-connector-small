#!/usr/bin/env python3
"""
Enhanced NodeGraph XML Generator with Topology Control
Creates connected graphs with specific topologies and validates socket connections.

Usage:
    python generate_graph.py chain --nodes 5
    python generate_graph.py mesh --nodes 8 --density 0.7
    python generate_graph.py tree --nodes 10 --max-children 3
    python generate_graph.py fully-connected --nodes 6
    python generate_graph.py random --nodes 12 --edges 20 --ensure-connected
"""

import argparse
import xml.etree.ElementTree as ET
import uuid
import random
import math
from collections import defaultdict, deque

class TopologyGenerator:
    def __init__(self):
        self.nodes = []
        self.edges = []
        
    def create_node(self, node_type, x, y, inputs=1, outputs=1):
        """Create a node with specified socket configuration"""
        if node_type not in ["IN", "OUT"]:
            raise ValueError(f"Invalid node type: {node_type}. Must be 'IN' or 'OUT'")
            
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
        """Create edge with full validation"""
        # Validate socket indices
        from_max = from_node['inputs'] + from_node['outputs'] - 1
        to_max = to_node['inputs'] - 1
        
        if from_socket_index > from_max or from_socket_index < from_node['inputs']:
            raise ValueError(f"Invalid from_socket_index {from_socket_index} for node {from_node['id']}")
        
        if to_socket_index > to_max or to_socket_index < 0:
            raise ValueError(f"Invalid to_socket_index {to_socket_index} for node {to_node['id']}")
        
        # Check for duplicate connections
        for edge in self.edges:
            if (edge['fromNode'] == from_node['id'] and 
                edge['toNode'] == to_node['id'] and
                edge['fromSocketIndex'] == from_socket_index and
                edge['toSocketIndex'] == to_socket_index):
                return None  # Duplicate, skip
        
        edge = {
            'id': str(uuid.uuid4()),
            'fromNode': from_node['id'],
            'toNode': to_node['id'],
            'fromSocketIndex': from_socket_index,
            'toSocketIndex': to_socket_index
        }
        self.edges.append(edge)
        return edge
    
    def get_available_output_socket(self, node):
        """Get next available output socket index"""
        used_outputs = set()
        for edge in self.edges:
            if edge['fromNode'] == node['id']:
                used_outputs.add(edge['fromSocketIndex'])
        
        # Find first unused output socket
        for i in range(node['outputs']):
            output_index = node['inputs'] + i
            if output_index not in used_outputs:
                return output_index
        return None
    
    def get_available_input_socket(self, node):
        """Get next available input socket index"""
        used_inputs = set()
        for edge in self.edges:
            if edge['toNode'] == node['id']:
                used_inputs.add(edge['toSocketIndex'])
        
        # Find first unused input socket
        for i in range(node['inputs']):
            if i not in used_inputs:
                return i
        return None
    
    def is_connected(self):
        """Check if graph is connected using BFS"""
        if len(self.nodes) <= 1:
            return True
            
        # Build adjacency list (undirected for connectivity check)
        adj = defaultdict(set)
        for edge in self.edges:
            adj[edge['fromNode']].add(edge['toNode'])
            adj[edge['toNode']].add(edge['fromNode'])
        
        # BFS from first node
        visited = set()
        queue = deque([self.nodes[0]['id']])
        visited.add(self.nodes[0]['id'])
        
        while queue:
            node_id = queue.popleft()
            for neighbor in adj[node_id]:
                if neighbor not in visited:
                    visited.add(neighbor)
                    queue.append(neighbor)
        
        return len(visited) == len(self.nodes)
    
    def ensure_connectivity(self):
        """Ensure graph is connected by adding minimal edges"""
        if self.is_connected():
            return
            
        # Find connected components
        components = []
        visited = set()
        
        for node in self.nodes:
            if node['id'] not in visited:
                component = []
                queue = deque([node['id']])
                
                while queue:
                    node_id = queue.popleft()
                    if node_id not in visited:
                        visited.add(node_id)
                        component.append(node_id)
                        
                        # Add neighbors
                        for edge in self.edges:
                            if edge['fromNode'] == node_id and edge['toNode'] not in visited:
                                queue.append(edge['toNode'])
                            if edge['toNode'] == node_id and edge['fromNode'] not in visited:
                                queue.append(edge['fromNode'])
                
                if component:
                    components.append(component)
        
        # Connect components
        node_dict = {n['id']: n for n in self.nodes}
        
        for i in range(len(components) - 1):
            # Find nodes to connect between components
            comp1_nodes = [node_dict[nid] for nid in components[i]]
            comp2_nodes = [node_dict[nid] for nid in components[i + 1]]
            
            # Find suitable connection
            connected = False
            for from_node in comp1_nodes:
                if connected:
                    break
                if from_node['outputs'] == 0:
                    continue
                    
                for to_node in comp2_nodes:
                    if to_node['inputs'] == 0:
                        continue
                        
                    from_socket = self.get_available_output_socket(from_node)
                    to_socket = self.get_available_input_socket(to_node)
                    
                    if from_socket is not None and to_socket is not None:
                        try:
                            self.create_edge(from_node, from_socket, to_node, to_socket)
                            connected = True
                            break
                        except ValueError:
                            continue
            
            if not connected:
                print(f"Warning: Could not connect component {i} to {i+1}")

    def generate_chain_topology(self, node_count):
        """Generate linear chain topology"""
        print(f"Generating chain topology with {node_count} nodes...")
        
        for i in range(node_count):
            x = 100 + i * 150
            y = 200
            
            if i == 0:
                # First node: pure output
                node = self.create_node("OUT", x, y, inputs=0, outputs=1)
            elif i == node_count - 1:
                # Last node: pure input  
                node = self.create_node("IN", x, y, inputs=1, outputs=0)
            else:
                # Middle nodes: input and output
                node = self.create_node("OUT", x, y, inputs=1, outputs=1)
        
        # Connect in chain
        for i in range(node_count - 1):
            from_node = self.nodes[i]
            to_node = self.nodes[i + 1]
            
            # from_node output socket is at index (inputs + 0)
            from_socket = from_node['inputs'] + 0
            # to_node input socket is at index 0
            to_socket = 0
            
            self.create_edge(from_node, from_socket, to_node, to_socket)

    def generate_tree_topology(self, node_count, max_children=3):
        """Generate tree topology"""
        print(f"Generating tree topology with {node_count} nodes, max {max_children} children...")
        
        if node_count == 0:
            return
            
        # Root node
        root = self.create_node("OUT", 300, 50, inputs=0, outputs=max_children)
        queue = [(root, 0)]  # (node, level)
        nodes_created = 1
        
        level_width = 200
        
        while queue and nodes_created < node_count:
            parent, level = queue.pop(0)
            children_count = min(max_children, node_count - nodes_created)
            
            for i in range(children_count):
                if nodes_created >= node_count:
                    break
                    
                # Position child
                x = 100 + (nodes_created % (2 ** (level + 1))) * level_width
                y = 100 + (level + 1) * 100
                
                # Last nodes are inputs, others are outputs with children
                remaining = node_count - nodes_created
                if remaining == 1 or random.random() < 0.3:  # Leaf node
                    child = self.create_node("IN", x, y, inputs=1, outputs=0)
                else:
                    child_children = min(max_children, remaining - 1)
                    child = self.create_node("OUT", x, y, inputs=1, outputs=child_children)
                    queue.append((child, level + 1))
                
                # Connect parent to child
                parent_output = parent['inputs'] + i
                child_input = 0
                
                try:
                    self.create_edge(parent, parent_output, child, child_input)
                except ValueError as e:
                    print(f"Tree connection error: {e}")
                
                nodes_created += 1

    def generate_star_topology(self, node_count):
        """Generate star topology with central hub"""
        print(f"Generating star topology with {node_count} nodes...")
        
        if node_count < 2:
            return
            
        # Central hub
        hub_outputs = node_count - 1
        hub = self.create_node("OUT", 300, 200, inputs=0, outputs=hub_outputs)
        
        # Spoke nodes in circle around hub
        radius = 150
        for i in range(node_count - 1):
            angle = (2 * math.pi * i) / (node_count - 1)
            x = 300 + int(radius * math.cos(angle))
            y = 200 + int(radius * math.sin(angle))
            
            spoke = self.create_node("IN", x, y, inputs=1, outputs=0)
            
            # Connect hub to spoke
            hub_output = hub['inputs'] + i  # Output socket index
            spoke_input = 0
            
            self.create_edge(hub, hub_output, spoke, spoke_input)

    def generate_mesh_topology(self, node_count, density=0.7):
        """Generate mesh topology with specified density"""
        print(f"Generating mesh topology with {node_count} nodes, density {density}...")
        
        # Create nodes in grid layout
        cols = int(math.ceil(math.sqrt(node_count)))
        max_connections = max(2, int(node_count * density))
        
        for i in range(node_count):
            row = i // cols
            col = i % cols
            x = 100 + col * 120
            y = 100 + row * 100
            
            # Most nodes can have inputs and outputs
            if i == 0:
                inputs, outputs = 0, max_connections
                node_type = "OUT"
            elif i == node_count - 1:
                inputs, outputs = max_connections, 0
                node_type = "IN"
            else:
                inputs = random.randint(1, min(3, max_connections))
                outputs = random.randint(1, min(3, max_connections))
                node_type = "OUT"
                
            self.create_node(node_type, x, y, inputs, outputs)
        
        # Create connections based on density
        max_possible_edges = node_count * (node_count - 1) // 2
        target_edges = int(max_possible_edges * density)
        
        attempts = 0
        while len(self.edges) < target_edges and attempts < target_edges * 10:
            from_node = random.choice(self.nodes)
            to_node = random.choice(self.nodes)
            
            if from_node == to_node or from_node['outputs'] == 0 or to_node['inputs'] == 0:
                attempts += 1
                continue
                
            from_socket = self.get_available_output_socket(from_node)
            to_socket = self.get_available_input_socket(to_node)
            
            if from_socket is not None and to_socket is not None:
                try:
                    self.create_edge(from_node, from_socket, to_node, to_socket)
                except ValueError:
                    pass
            
            attempts += 1
        
        # Ensure connectivity
        self.ensure_connectivity()

    def generate_ring_topology(self, node_count):
        """Generate ring topology"""
        print(f"Generating ring topology with {node_count} nodes...")
        
        # Create nodes in circle
        radius = 150
        center_x, center_y = 300, 200
        
        for i in range(node_count):
            angle = (2 * math.pi * i) / node_count
            x = center_x + int(radius * math.cos(angle))
            y = center_y + int(radius * math.sin(angle))
            
            # All nodes need input and output for ring
            self.create_node("OUT", x, y, inputs=1, outputs=1)
        
        # Connect in ring
        for i in range(node_count):
            from_node = self.nodes[i]
            to_node = self.nodes[(i + 1) % node_count]
            
            from_socket = from_node['inputs'] + 0  # First output
            to_socket = 0  # First input
            
            self.create_edge(from_node, from_socket, to_node, to_socket)

    def to_xml(self):
        """Convert to XML format"""
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
        """Save XML with pretty formatting"""
        root = self.to_xml()
        ET.indent(root, space="  ", level=0)
        tree = ET.ElementTree(root)
        tree.write(filename, encoding='utf-8', xml_declaration=True)
        
        # Print summary
        connected = "✅ Connected" if self.is_connected() else "❌ Disconnected"
        print(f"Generated: {filename}")
        print(f"  Nodes: {len(self.nodes)}")
        print(f"  Edges: {len(self.edges)}")
        print(f"  Status: {connected}")

def main():
    parser = argparse.ArgumentParser(description='Generate NodeGraph XML with topology control')
    
    # Topology types
    subparsers = parser.add_subparsers(dest='topology', help='Graph topology')
    
    # Chain topology
    chain_parser = subparsers.add_parser('chain', help='Linear chain of nodes')
    chain_parser.add_argument('--nodes', type=int, default=5, help='Number of nodes')
    
    # Tree topology
    tree_parser = subparsers.add_parser('tree', help='Tree structure')
    tree_parser.add_argument('--nodes', type=int, default=7, help='Number of nodes')
    tree_parser.add_argument('--max-children', type=int, default=3, help='Max children per node')
    
    # Star topology
    star_parser = subparsers.add_parser('star', help='Hub and spoke')
    star_parser.add_argument('--nodes', type=int, default=6, help='Number of nodes')
    
    # Mesh topology
    mesh_parser = subparsers.add_parser('mesh', help='Dense mesh connectivity')
    mesh_parser.add_argument('--nodes', type=int, default=8, help='Number of nodes')
    mesh_parser.add_argument('--density', type=float, default=0.7, help='Connection density (0.0-1.0)')
    
    # Ring topology
    ring_parser = subparsers.add_parser('ring', help='Circular ring')
    ring_parser.add_argument('--nodes', type=int, default=6, help='Number of nodes')
    
    # Custom topology
    custom_parser = subparsers.add_parser('custom', help='Custom random topology')
    custom_parser.add_argument('--nodes', type=int, default=10, help='Number of nodes')
    custom_parser.add_argument('--edges', type=int, default=15, help='Number of edges')
    custom_parser.add_argument('--ensure-connected', action='store_true', help='Ensure graph is connected')
    
    # Common options
    for p in [chain_parser, tree_parser, star_parser, mesh_parser, ring_parser, custom_parser]:
        p.add_argument('--output', '-o', help='Output filename (default: auto-generated)')
        p.add_argument('--seed', type=int, help='Random seed for reproducible graphs')
    
    args = parser.parse_args()
    
    if not args.topology:
        parser.print_help()
        return
    
    # Set random seed if provided
    if args.seed:
        random.seed(args.seed)
        print(f"Using random seed: {args.seed}")
    
    generator = TopologyGenerator()
    
    # Generate based on topology
    if args.topology == 'chain':
        generator.generate_chain_topology(args.nodes)
        default_filename = f'chain_{args.nodes}nodes.xml'
        
    elif args.topology == 'tree':
        generator.generate_tree_topology(args.nodes, args.max_children)
        default_filename = f'tree_{args.nodes}nodes_{args.max_children}children.xml'
        
    elif args.topology == 'star':
        generator.generate_star_topology(args.nodes)
        default_filename = f'star_{args.nodes}nodes.xml'
        
    elif args.topology == 'mesh':
        generator.generate_mesh_topology(args.nodes, args.density)
        default_filename = f'mesh_{args.nodes}nodes_{args.density}density.xml'
        
    elif args.topology == 'ring':
        generator.generate_ring_topology(args.nodes)
        default_filename = f'ring_{args.nodes}nodes.xml'
        
    elif args.topology == 'custom':
        # Custom implementation would go here
        print("Custom topology not yet implemented")
        return
    
    # Save the graph
    filename = args.output or default_filename
    generator.save_xml(filename)
    
    print(f"\nTo test with NodeGraph:")
    print(f"./build_Debug/NodeGraph.exe {filename}")

if __name__ == '__main__':
    main()