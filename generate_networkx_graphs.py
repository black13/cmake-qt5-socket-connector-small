#!/usr/bin/env python3
"""
NetworkX Graph Generator for Self-Serializing Node/Edge System

Uses NetworkX to create proper graphs, then exports to XML format.
Focuses on performance measurement and document integrity testing.
"""

import networkx as nx
import uuid
import xml.etree.ElementTree as ET
import random
import math
import time
import os

class GraphXMLGenerator:
    def __init__(self):
        self.performance_log = []
    
    def log_performance(self, operation, duration, details):
        """Log performance metrics for optimization"""
        entry = {
            'operation': operation,
            'duration_ms': round(duration * 1000, 3),
            'details': details,
            'timestamp': time.time()
        }
        self.performance_log.append(entry)
        print(f"⏱️  {operation}: {entry['duration_ms']}ms - {details}")
    
    def networkx_to_xml(self, graph, node_positions=None):
        """Convert NetworkX graph to our XML format with performance logging"""
        start_time = time.time()
        
        root = ET.Element("graph")
        root.set("version", "1.0")
        root.set("created", str(int(time.time())))
        
        # Add nodes with sockets
        nodes_elem = ET.SubElement(root, "nodes")
        socket_map = {}  # Track sockets for edge creation
        
        for node_id in graph.nodes():
            node_data = graph.nodes[node_id]
            node_elem = ET.SubElement(nodes_elem, "node")
            node_elem.set("id", str(node_id))
            
            # Position from layout or random
            if node_positions and node_id in node_positions:
                x, y = node_positions[node_id]
                node_elem.set("x", str(int(x * 100)))  # Scale up for pixels
                node_elem.set("y", str(int(y * 100)))
            else:
                node_elem.set("x", str(random.randint(50, 800)))
                node_elem.set("y", str(random.randint(50, 600)))
            
            # Add type if specified
            if 'type' in node_data:
                node_elem.set("type", node_data['type'])
            
            # Create sockets based on node degree
            degree = graph.degree(node_id)
            sockets = []
            
            # Create sockets around node perimeter
            for i in range(max(2, degree + 1)):  # At least 2 sockets
                angle = (2 * math.pi * i) / max(2, degree + 1)
                rel_x = int(40 * math.cos(angle))
                rel_y = int(40 * math.sin(angle))
                
                socket_id = str(uuid.uuid4())
                socket_elem = ET.SubElement(node_elem, "socket")
                socket_elem.set("id", socket_id)
                socket_elem.set("rel_x", str(rel_x))
                socket_elem.set("rel_y", str(rel_y))
                
                # Add socket type for future factory usage
                socket_elem.set("type", "bidirectional")
                
                sockets.append(socket_id)
            
            socket_map[node_id] = sockets
        
        # Add edges
        edges_elem = ET.SubElement(root, "edges")
        for edge in graph.edges(data=True):
            from_node, to_node, edge_data = edge
            
            edge_elem = ET.SubElement(edges_elem, "edge")
            edge_elem.set("id", str(uuid.uuid4()))
            
            # Use first available sockets from each node
            from_socket = socket_map[from_node][0] if socket_map[from_node] else None
            to_socket = socket_map[to_node][0] if socket_map[to_node] else None
            
            if from_socket and to_socket:
                edge_elem.set("from_socket", from_socket)
                edge_elem.set("to_socket", to_socket)
                
                # Add edge type for future factory usage
                if 'type' in edge_data:
                    edge_elem.set("type", edge_data['type'])
                else:
                    edge_elem.set("type", "connection")
        
        duration = time.time() - start_time
        self.log_performance("NetworkX->XML", duration, 
                           f"{len(graph.nodes())} nodes, {len(graph.edges())} edges")
        
        return root
    
    def save_xml_with_timing(self, root, filename):
        """Save XML with performance measurement"""
        start_time = time.time()
        
        # Pretty format
        self._indent(root)
        tree = ET.ElementTree(root)
        
        with open(filename, 'wb') as f:
            tree.write(f, encoding='utf-8', xml_declaration=True)
        
        # Measure file size for performance tracking
        file_size = os.path.getsize(filename)
        duration = time.time() - start_time
        
        self.log_performance("XML Save", duration, f"{filename}, {file_size} bytes")
        
        return file_size
    
    def _indent(self, elem, level=0):
        """Pretty print XML"""
        i = "\n" + level * "  "
        if len(elem):
            if not elem.text or not elem.text.strip():
                elem.text = i + "  "
            if not elem.tail or not elem.tail.strip():
                elem.tail = i
            for elem in elem:
                self._indent(elem, level + 1)
            if not elem.tail or not elem.tail.strip():
                elem.tail = i
        else:
            if level and (not elem.tail or not elem.tail.strip()):
                elem.tail = i

def generate_test_graphs():
    """Generate various graph types using NetworkX"""
    generator = GraphXMLGenerator()
    
    print("🚀 Generating test graphs with NetworkX...")
    
    # 1. Simple path graph
    print("\n📈 Simple Path Graph:")
    G = nx.path_graph(5)
    # Add node types for factory testing
    for i, node in enumerate(G.nodes()):
        G.nodes[node]['type'] = 'input' if i == 0 else 'output' if i == 4 else 'processor'
    
    pos = nx.spring_layout(G)
    root = generator.networkx_to_xml(G, pos)
    generator.save_xml_with_timing(root, "graph_path.xml")
    
    # 2. Complete graph (all nodes connected)
    print("\n🔗 Complete Graph:")
    G = nx.complete_graph(6)
    for node in G.nodes():
        G.nodes[node]['type'] = 'hub'
    for edge in G.edges():
        G.edges[edge]['type'] = 'full_mesh'
    
    pos = nx.circular_layout(G)
    root = generator.networkx_to_xml(G, pos)
    generator.save_xml_with_timing(root, "graph_complete.xml")
    
    # 3. Grid graph
    print("\n🔲 Grid Graph:")
    G = nx.grid_2d_graph(4, 5)
    # Relabel nodes to have simple IDs
    mapping = {node: f"grid_{node[0]}_{node[1]}" for node in G.nodes()}
    G = nx.relabel_nodes(G, mapping)
    
    for node in G.nodes():
        G.nodes[node]['type'] = 'grid_cell'
    
    pos = {node: (int(node.split('_')[1]), int(node.split('_')[2])) for node in G.nodes()}
    root = generator.networkx_to_xml(G, pos)
    generator.save_xml_with_timing(root, "graph_grid.xml")
    
    # 4. Random graph (Erdős–Rényi)
    print("\n🎲 Random Graph:")
    G = nx.erdos_renyi_graph(15, 0.3)
    for node in G.nodes():
        G.nodes[node]['type'] = random.choice(['processor', 'filter', 'aggregator'])
    for edge in G.edges():
        G.edges[edge]['type'] = random.choice(['data_flow', 'control', 'feedback'])
    
    pos = nx.spring_layout(G)
    root = generator.networkx_to_xml(G, pos)
    generator.save_xml_with_timing(root, "graph_random.xml")
    
    # 5. Scale-free network (Barabási–Albert)
    print("\n📊 Scale-Free Network:")
    G = nx.barabasi_albert_graph(20, 3)
    for node in G.nodes():
        degree = G.degree(node)
        if degree > 10:
            G.nodes[node]['type'] = 'hub'
        elif degree > 5:
            G.nodes[node]['type'] = 'connector'
        else:
            G.nodes[node]['type'] = 'endpoint'
    
    pos = nx.spring_layout(G)
    root = generator.networkx_to_xml(G, pos)
    generator.save_xml_with_timing(root, "graph_scale_free.xml")
    
    # 6. Tree graph
    print("\n🌳 Tree Graph:")
    G = nx.balanced_tree(2, 3)  # Binary tree with 3 levels
    # Set root and assign types
    root_node = 0
    G.nodes[root_node]['type'] = 'root'
    for node in G.nodes():
        if node != root_node:
            if G.degree(node) == 1:
                G.nodes[node]['type'] = 'leaf'
            else:
                G.nodes[node]['type'] = 'branch'
    
    pos = nx.spring_layout(G)
    root_xml = generator.networkx_to_xml(G, pos)
    generator.save_xml_with_timing(root_xml, "graph_tree.xml")
    
    # 7. Large stress test graph
    print("\n💪 Large Stress Test:")
    G = nx.erdos_renyi_graph(50, 0.1)
    for node in G.nodes():
        G.nodes[node]['type'] = f"type_{random.randint(1,5)}"
    for edge in G.edges():
        G.edges[edge]['type'] = f"edge_type_{random.randint(1,3)}"
    
    pos = nx.spring_layout(G)
    root = generator.networkx_to_xml(G, pos)
    file_size = generator.save_xml_with_timing(root, "graph_stress.xml")
    
    # Performance summary
    print(f"\n📊 Performance Summary:")
    total_time = sum(entry['duration_ms'] for entry in generator.performance_log)
    print(f"Total generation time: {total_time:.1f}ms")
    
    xml_operations = [e for e in generator.performance_log if 'XML' in e['operation']]
    avg_xml_time = sum(e['duration_ms'] for e in xml_operations) / len(xml_operations)
    print(f"Average XML operation: {avg_xml_time:.1f}ms")
    
    print(f"\n📁 Generated files:")
    test_files = [
        "graph_path.xml", "graph_complete.xml", "graph_grid.xml", 
        "graph_random.xml", "graph_scale_free.xml", "graph_tree.xml", "graph_stress.xml"
    ]
    
    for filename in test_files:
        if os.path.exists(filename):
            size = os.path.getsize(filename)
            print(f"  {filename}: {size} bytes")
    
    print(f"\n🎯 Use these files to test:")
    print(f"  • XML parsing performance")
    print(f"  • Node/Socket/Edge self-serialization")
    print(f"  • Factory type creation")
    print(f"  • Observer pattern efficiency")
    print(f"  • Document integrity during modifications")

if __name__ == "__main__":
    generate_test_graphs()