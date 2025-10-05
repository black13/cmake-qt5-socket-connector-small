#!/usr/bin/env python3
"""
analyze_test_coverage.py - Identify untested code paths in Qt5 Node Graph Editor

Uses libclang to parse C++ code and identify:
- Public methods without corresponding tests
- Critical functions that need testing
- Test coverage gaps by class
- Priority areas for test development

Usage:
    python3 analyze_test_coverage.py [options]

Options:
    --source-dir DIR    Source directory (default: current dir)
    --test-dir DIR      Test directory (default: tests/)
    --verbose           Show detailed analysis
    --json              Output as JSON
"""

import sys
import os
from pathlib import Path
from collections import defaultdict
import json
from typing import Dict, List, Set, Tuple
from clang.cindex import Index, CursorKind, AccessSpecifier, TranslationUnit

class CodeCoverageAnalyzer:
    """Analyzes C++ code to identify untested functions and methods"""

    def __init__(self, source_dir: str, test_dir: str = "tests", verbose: bool = False):
        self.source_dir = Path(source_dir)
        self.test_dir = Path(test_dir)
        self.verbose = verbose
        self.index = Index.create()

        # Data structures
        self.classes: Dict[str, List[dict]] = defaultdict(list)
        self.functions: List[dict] = []
        self.test_references: Set[str] = set()
        self.coverage_report: Dict[str, dict] = {}

    def log(self, msg: str):
        """Print message if verbose mode"""
        if self.verbose:
            print(f"[ANALYZER] {msg}")

    def parse_cpp_file(self, filepath: Path) -> TranslationUnit:
        """Parse C++ file with clang"""
        self.log(f"Parsing {filepath.name}")

        # Compilation arguments for Qt5 + C++17
        args = [
            '-x', 'c++',
            '-std=c++17',
            '-I/usr/include/qt5',
            '-I/usr/include/qt5/QtCore',
            '-I/usr/include/qt5/QtWidgets',
            '-I/usr/include/qt5/QtGui',
            '-I/usr/include/libxml2',
            '-DQT_CORE_LIB',
            '-DQT_WIDGETS_LIB',
            '-DQT_GUI_LIB',
            '-DUSE_LIBXML2',
        ]

        try:
            tu = self.index.parse(str(filepath), args=args)
            if tu.diagnostics:
                for diag in tu.diagnostics:
                    if diag.severity >= 3:  # Error or Fatal
                        self.log(f"  Warning: {diag.spelling}")
            return tu
        except Exception as e:
            self.log(f"  ERROR parsing {filepath}: {e}")
            return None

    def get_method_signature(self, cursor) -> str:
        """Extract method signature"""
        params = []
        for arg in cursor.get_arguments():
            param_type = arg.type.spelling
            param_name = arg.spelling
            params.append(f"{param_type} {param_name}" if param_name else param_type)

        return_type = cursor.result_type.spelling
        method_name = cursor.spelling
        param_str = ", ".join(params)

        return f"{return_type} {method_name}({param_str})"

    def is_public_method(self, cursor) -> bool:
        """Check if method is public"""
        return cursor.access_specifier == AccessSpecifier.PUBLIC

    def is_test_worthy(self, cursor) -> bool:
        """Determine if method should be tested"""
        name = cursor.spelling

        # Skip destructors, constructors, Qt meta methods
        skip_patterns = [
            'qt_',
            'moc_',
            'operator',
            'staticMetaObject',
            'metaObject',
            'tr',
            'trUtf8'
        ]

        for pattern in skip_patterns:
            if pattern in name.lower():
                return False

        # Skip getters/setters (often trivial)
        if name.startswith('get') or name.startswith('set') or name.startswith('is'):
            if cursor.result_type.spelling in ['bool', 'int', 'QString', 'QUuid']:
                return False

        return True

    def is_project_file(self, filepath: str) -> bool:
        """Check if file belongs to this project (not system headers)"""
        if not filepath:
            return False

        # Filter system headers and STL includes
        system_patterns = [
            '/usr/include', '/lib/gcc', 'c++/11', 'bits/', 'ext/',
            'pthread.h', 'unicode/', 'uenum.h', 'ucnv.h'
        ]

        for pattern in system_patterns:
            if pattern in filepath:
                return False

        # Project files are simple names or start with ./
        return True

    def extract_class_methods(self, cursor, class_name: str = ""):
        """Recursively extract class methods"""

        if cursor.kind == CursorKind.CLASS_DECL:
            class_name = cursor.spelling
            if not class_name:
                return

            # Only process classes defined in project files
            if cursor.location.file:
                if not self.is_project_file(cursor.location.file.name):
                    return

            self.log(f"  Found class: {class_name}")

        if cursor.kind == CursorKind.CXX_METHOD and class_name:
            if self.is_public_method(cursor) and self.is_test_worthy(cursor):
                signature = self.get_method_signature(cursor)
                location = cursor.location

                method_info = {
                    'class': class_name,
                    'name': cursor.spelling,
                    'signature': signature,
                    'file': location.file.name if location.file else 'unknown',
                    'line': location.line,
                    'is_const': cursor.is_const_method(),
                    'is_static': cursor.is_static_method(),
                    'is_virtual': cursor.is_virtual_method(),
                }

                self.classes[class_name].append(method_info)
                self.log(f"    Method: {cursor.spelling}() - line {location.line}")

        # Free functions (not in a class)
        if cursor.kind == CursorKind.FUNCTION_DECL and not class_name:
            # Only process functions in project files
            if cursor.location.file and not self.is_project_file(cursor.location.file.name):
                pass  # Skip system functions
            elif self.is_test_worthy(cursor):
                signature = self.get_method_signature(cursor)
                location = cursor.location

                func_info = {
                    'name': cursor.spelling,
                    'signature': signature,
                    'file': location.file.name if location.file else 'unknown',
                    'line': location.line,
                }

                self.functions.append(func_info)
                self.log(f"  Function: {cursor.spelling}() - line {location.line}")

        # Recurse into children
        for child in cursor.get_children():
            self.extract_class_methods(child, class_name)

    def scan_test_references(self):
        """Scan test directory for function/method references"""
        if not self.test_dir.exists():
            self.log(f"Test directory not found: {self.test_dir}")
            return

        self.log(f"Scanning test directory: {self.test_dir}")

        # For now, simple text search in test files
        # TODO: Parse test files with clang for more accuracy
        for test_file in self.test_dir.rglob("*.cpp"):
            try:
                with open(test_file, 'r', encoding='utf-8') as f:
                    content = f.read()

                    # Extract function calls (simple heuristic)
                    for class_name, methods in self.classes.items():
                        for method in methods:
                            method_name = method['name']
                            # Look for method calls: obj.method() or Class::method()
                            if f".{method_name}(" in content or f"::{method_name}(" in content:
                                self.test_references.add(f"{class_name}::{method_name}")

                    # Check free functions
                    for func in self.functions:
                        func_name = func['name']
                        if f"{func_name}(" in content:
                            self.test_references.add(func_name)

            except Exception as e:
                self.log(f"  Error reading {test_file}: {e}")

    def analyze_source_files(self):
        """Analyze all C++ source files"""
        # Only analyze header files for class definitions
        cpp_files = list(self.source_dir.glob("*.h"))

        # Filter out build directories and generated files
        cpp_files = [f for f in cpp_files if 'build' not in str(f) and 'moc_' not in f.name]

        # Focus on core project files only
        project_prefixes = [
            'node', 'edge', 'socket', 'scene', 'graph', 'window',
            'view', 'ghost', 'qgraph', 'observer', 'factory', 'registry'
        ]

        cpp_files = [
            f for f in cpp_files
            if any(f.stem.lower().startswith(prefix) for prefix in project_prefixes)
        ]

        self.log(f"Found {len(cpp_files)} project source files")

        for filepath in cpp_files:
            tu = self.parse_cpp_file(filepath)
            if tu and tu.cursor:
                self.extract_class_methods(tu.cursor)

    def generate_coverage_report(self):
        """Generate test coverage report"""
        self.log("Generating coverage report...")

        # Analyze class coverage
        for class_name, methods in self.classes.items():
            tested_methods = []
            untested_methods = []

            for method in methods:
                method_ref = f"{class_name}::{method['name']}"
                if method_ref in self.test_references:
                    tested_methods.append(method)
                else:
                    untested_methods.append(method)

            total = len(methods)
            tested_count = len(tested_methods)
            coverage_pct = (tested_count / total * 100) if total > 0 else 0

            self.coverage_report[class_name] = {
                'total_methods': total,
                'tested_methods': tested_count,
                'untested_methods': len(untested_methods),
                'coverage_percent': coverage_pct,
                'untested_list': untested_methods,
                'tested_list': tested_methods,
            }

        # Analyze free function coverage
        tested_funcs = []
        untested_funcs = []

        for func in self.functions:
            if func['name'] in self.test_references:
                tested_funcs.append(func)
            else:
                untested_funcs.append(func)

        total_funcs = len(self.functions)
        tested_funcs_count = len(tested_funcs)
        func_coverage_pct = (tested_funcs_count / total_funcs * 100) if total_funcs > 0 else 0

        self.coverage_report['_free_functions'] = {
            'total_methods': total_funcs,
            'tested_methods': tested_funcs_count,
            'untested_methods': len(untested_funcs),
            'coverage_percent': func_coverage_pct,
            'untested_list': untested_funcs,
            'tested_list': tested_funcs,
        }

    def print_report(self, output_json: bool = False):
        """Print coverage report"""
        # Filter to only project classes (not system headers)
        project_classes = {}
        for class_name, report in self.coverage_report.items():
            if class_name == '_free_functions':
                project_classes[class_name] = report
                continue

            # Check if this class is from project files
            if report['untested_list'] or report['tested_list']:
                sample_method = report['untested_list'][0] if report['untested_list'] else report['tested_list'][0]
                if self.is_project_file(sample_method['file']):
                    project_classes[class_name] = report

        if output_json:
            print(json.dumps(project_classes, indent=2, default=str))
            return

        print("\n" + "="*80)
        print("TEST COVERAGE ANALYSIS - Qt5 Node Graph Editor")
        print("="*80 + "\n")

        # Overall statistics (project only)
        total_items = sum(r['total_methods'] for r in project_classes.values())
        total_tested = sum(r['tested_methods'] for r in project_classes.values())
        overall_coverage = (total_tested / total_items * 100) if total_items > 0 else 0

        print(f"Overall Coverage: {total_tested}/{total_items} ({overall_coverage:.1f}%)")
        print(f"Total Classes: {len(project_classes) - 1}")  # -1 for _free_functions
        print(f"Total Methods: {total_items}")
        print()

        # Per-class coverage
        print("-"*80)
        print("CLASS COVERAGE SUMMARY")
        print("-"*80)

        # Sort by coverage percentage (lowest first - needs most work)
        sorted_classes = sorted(
            [(k, v) for k, v in project_classes.items() if k != '_free_functions'],
            key=lambda x: x[1]['coverage_percent']
        )

        for class_name, report in sorted_classes:
            coverage = report['coverage_percent']
            tested = report['tested_methods']
            total = report['total_methods']

            status = "✓" if coverage >= 80 else "⚠" if coverage >= 50 else "✗"
            print(f"{status} {class_name:30s} {tested:3d}/{total:3d} ({coverage:5.1f}%)")

        print()
        print("-"*80)
        print("PRIORITY AREAS NEEDING TESTS (Coverage < 50%)")
        print("-"*80)

        for class_name, report in sorted_classes:
            if report['coverage_percent'] < 50 and report['untested_methods'] > 0:
                print(f"\n{class_name} ({report['coverage_percent']:.1f}% coverage):")
                print(f"  File: {report['untested_list'][0]['file']}")
                print(f"  Untested methods ({report['untested_methods']}):")

                for method in report['untested_list'][:5]:  # Show first 5
                    print(f"    • {method['name']}() - line {method['line']}")

                if report['untested_methods'] > 5:
                    print(f"    ... and {report['untested_methods'] - 5} more")

        print("\n" + "="*80)

    def run(self, output_json: bool = False):
        """Run full analysis"""
        print("Starting test coverage analysis...")

        self.analyze_source_files()
        self.scan_test_references()
        self.generate_coverage_report()
        self.print_report(output_json)


def main():
    import argparse

    parser = argparse.ArgumentParser(
        description='Analyze test coverage for Qt5 Node Graph Editor'
    )
    parser.add_argument('--source-dir', default='.', help='Source directory')
    parser.add_argument('--test-dir', default='tests', help='Test directory')
    parser.add_argument('--verbose', action='store_true', help='Verbose output')
    parser.add_argument('--json', action='store_true', help='Output as JSON')

    args = parser.parse_args()

    analyzer = CodeCoverageAnalyzer(
        source_dir=args.source_dir,
        test_dir=args.test_dir,
        verbose=args.verbose
    )

    analyzer.run(output_json=args.json)


if __name__ == '__main__':
    main()
