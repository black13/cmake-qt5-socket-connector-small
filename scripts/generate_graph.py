#!/usr/bin/env python3
"""
generate_graph.py - helper to create sample NodeGraph XML files.

Usage examples:
    python3 scripts/generate_graph.py pipeline.xml --pattern pipeline --stages 4
    python3 scripts/generate_graph.py split_merge.xml --pattern split-merge
"""

from __future__ import annotations

import argparse
import sys
import uuid
import xml.etree.ElementTree as ET
from dataclasses import dataclass
from pathlib import Path
from typing import List, Tuple


@dataclass
class Node:
    """Represents a serialized node entry."""

    id: str
    type: str
    x: float
    y: float
    inputs: int
    outputs: int


@dataclass
class Edge:
    """Represents a serialized edge entry."""

    id: str
    from_node: Node
    to_node: Node
    from_socket: int
    to_socket: int


def _new_id() -> str:
    return str(uuid.uuid4())


def build_pipeline(stages: int, spacing: int) -> Tuple[List[Node], List[Edge]]:
    """
    Pipeline: SOURCE -> TRANSFORM x N -> SINK
    """
    if stages < 0:
        raise ValueError("stages must be >= 0")

    nodes: List[Node] = []
    edges: List[Edge] = []

    x = 0
    source = Node(_new_id(), "SOURCE", x, 0, inputs=0, outputs=1)
    nodes.append(source)

    prev = source
    for _ in range(stages):
        x += spacing
        stage = Node(_new_id(), "TRANSFORM", x, 0, inputs=1, outputs=1)
        nodes.append(stage)
        edges.append(Edge(_new_id(), prev, stage, from_socket=0, to_socket=0))
        prev = stage

    x += spacing
    sink = Node(_new_id(), "SINK", x, 0, inputs=1, outputs=0)
    nodes.append(sink)
    edges.append(Edge(_new_id(), prev, sink, from_socket=0, to_socket=0))

    return nodes, edges


def build_split_merge(spacing: int) -> Tuple[List[Node], List[Edge]]:
    """
    Split/merge demo: SOURCE -> SPLIT -> two TRANSFORM -> MERGE -> SINK
    """
    nodes: List[Node] = []
    edges: List[Edge] = []

    source = Node(_new_id(), "SOURCE", 0, 0, 0, 1)
    split = Node(_new_id(), "SPLIT", spacing, 0, 1, 2)
    upper = Node(_new_id(), "TRANSFORM", spacing * 2, -spacing, 1, 1)
    lower = Node(_new_id(), "TRANSFORM", spacing * 2, spacing, 1, 1)
    merge = Node(_new_id(), "MERGE", spacing * 3, 0, 2, 1)
    sink = Node(_new_id(), "SINK", spacing * 4, 0, 1, 0)

    nodes.extend([source, split, upper, lower, merge, sink])

    edges.extend(
        [
            Edge(_new_id(), source, split, 0, 0),
            Edge(_new_id(), split, upper, 1, 0),
            Edge(_new_id(), split, lower, 2, 0),
            Edge(_new_id(), upper, merge, 0, 0),
            Edge(_new_id(), lower, merge, 0, 1),
            Edge(_new_id(), merge, sink, 0, 0),
        ]
    )

    return nodes, edges


def write_graph(nodes: List[Node], edges: List[Edge], path: Path) -> None:
    root = ET.Element("graph", attrib={"version": "1.0"})

    for node in nodes:
        ET.SubElement(
            root,
            "node",
            attrib={
                "id": node.id,
                "x": f"{int(node.x)}",
                "y": f"{int(node.y)}",
                "type": node.type,
                "inputs": str(node.inputs),
                "outputs": str(node.outputs),
            },
        )

    for edge in edges:
        ET.SubElement(
            root,
            "edge",
            attrib={
                "id": edge.id,
                "fromNode": edge.from_node.id,
                "toNode": edge.to_node.id,
                "fromSocketIndex": str(edge.from_socket),
                "toSocketIndex": str(edge.to_socket),
            },
        )

    tree = ET.ElementTree(root)
    path.parent.mkdir(parents=True, exist_ok=True)
    tree.write(path, encoding="utf-8", xml_declaration=True)


def parse_args(argv: List[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Generate sample NodeGraph XML")
    parser.add_argument("output", type=Path, help="Output XML file")
    parser.add_argument(
        "--pattern",
        choices=("pipeline", "split-merge"),
        default="pipeline",
        help="Graph pattern to emit",
    )
    parser.add_argument(
        "--stages",
        type=int,
        default=3,
        help="Number of transform stages (pipeline pattern only)",
    )
    parser.add_argument(
        "--spacing",
        type=int,
        default=150,
        help="Pixel spacing between generated nodes",
    )
    return parser.parse_args(argv)


def main(argv: List[str]) -> int:
    args = parse_args(argv)

    if args.pattern == "pipeline":
        nodes, edges = build_pipeline(stages=args.stages, spacing=args.spacing)
    else:
        nodes, edges = build_split_merge(spacing=args.spacing)

    write_graph(nodes, edges, args.output)
    print(f"Wrote {len(nodes)} nodes / {len(edges)} edges to {args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
