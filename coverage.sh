#!/bin/bash
#
# Code Coverage Analysis Script
# Builds with coverage instrumentation and generates HTML reports
#
# Usage:
#   ./coverage.sh               # Build, run tests, generate report
#   ./coverage.sh build         # Build with coverage only
#   ./coverage.sh report        # Generate report from existing data
#   ./coverage.sh clean         # Remove coverage data
#

set -e  # Exit on error

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build_linux"
COVERAGE_DIR="${SCRIPT_DIR}/coverage_html"
PROFILE_DATA="${BUILD_DIR}/nodegraph.profdata"
EXECUTABLE="${BUILD_DIR}/NodeGraph"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

function print_header() {
    echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    echo -e "${BLUE}$1${NC}"
    echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
}

function print_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

function print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

function print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

function check_compiler() {
    if ! command -v clang++ &> /dev/null; then
        print_error "clang++ not found. Coverage requires Clang compiler."
        exit 1
    fi

    if ! command -v llvm-cov &> /dev/null; then
        print_error "llvm-cov not found. Install LLVM tools."
        exit 1
    fi

    if ! command -v llvm-profdata &> /dev/null; then
        print_error "llvm-profdata not found. Install LLVM tools."
        exit 1
    fi

    print_info "Found Clang $(clang++ --version | head -1)"
    print_info "Found llvm-cov $(llvm-cov --version | head -1)"
}

function build_with_coverage() {
    print_header "Building with Code Coverage"

    check_compiler

    # Create build directory
    mkdir -p "${BUILD_DIR}"
    cd "${BUILD_DIR}"

    # Configure with coverage enabled
    print_info "Configuring CMake with coverage enabled..."
    cmake -DCMAKE_BUILD_TYPE=Debug \
          -DCMAKE_CXX_COMPILER=clang++ \
          -DCMAKE_C_COMPILER=clang \
          -DENABLE_COVERAGE=ON \
          ..

    # Build
    print_info "Building project..."
    make -j$(nproc)

    print_info "Build complete: ${EXECUTABLE}"
}

function run_tests() {
    print_header "Running Tests to Generate Coverage Data"

    if [[ ! -f "${EXECUTABLE}" ]]; then
        print_error "Executable not found: ${EXECUTABLE}"
        print_error "Run './coverage.sh build' first"
        exit 1
    fi

    cd "${BUILD_DIR}"

    # Set environment variable for coverage data output
    export LLVM_PROFILE_FILE="nodegraph-%p.profraw"

    print_info "Running test scenarios for coverage..."
    print_info "Coverage data will be written to: ${LLVM_PROFILE_FILE}"

    # Need DISPLAY for Qt GUI (use headless X server or Xvfb if available)
    export DISPLAY=${DISPLAY:-:0}

    # Test 1: Load existing XML file (exercises deserialization path)
    if [[ -f "../tests_medium.xml" ]]; then
        print_info "Test 1: Loading medium graph from XML (500 nodes)..."
        timeout 15s "${EXECUTABLE}" ../tests_medium.xml || true
    elif [[ -f "../tests_small.xml" ]]; then
        print_info "Test 1: Loading small graph from XML (100 nodes)..."
        timeout 15s "${EXECUTABLE}" ../tests_small.xml || true
    fi

    # Test 2: JavaScript tests (exercises creation path)
    print_info "Test 2: Running edge curve guards test..."
    timeout 30s "${EXECUTABLE}" --script ../scripts/test_edge_curve_guards.js || true

    # Test 3: Geometry discipline test if it exists
    if [[ -f "../scripts/test_geometry_discipline.js" ]]; then
        print_info "Test 3: Running geometry discipline test..."
        timeout 30s "${EXECUTABLE}" --script ../scripts/test_geometry_discipline.js || true
    fi

    # Check if any .profraw files were generated
    PROFRAW_COUNT=$(ls -1 nodegraph-*.profraw 2>/dev/null | wc -l)
    if [[ ${PROFRAW_COUNT} -eq 0 ]]; then
        print_warning "No .profraw files generated. Tests may not have run."
        return 1
    fi

    print_info "Generated ${PROFRAW_COUNT} coverage data file(s)"
}

function merge_coverage_data() {
    print_header "Merging Coverage Data"

    cd "${BUILD_DIR}"

    # Check for .profraw files
    if ! ls nodegraph-*.profraw 1> /dev/null 2>&1; then
        print_error "No .profraw files found in ${BUILD_DIR}"
        print_error "Run './coverage.sh' or './coverage.sh test' first"
        exit 1
    fi

    print_info "Merging coverage files..."
    llvm-profdata merge -sparse nodegraph-*.profraw -o "${PROFILE_DATA}"

    print_info "Coverage data merged: ${PROFILE_DATA}"
}

function generate_report() {
    print_header "Generating Coverage Report"

    if [[ ! -f "${PROFILE_DATA}" ]]; then
        print_error "Coverage data not found: ${PROFILE_DATA}"
        print_error "Run './coverage.sh' or merge data first"
        exit 1
    fi

    # Create coverage directory
    mkdir -p "${COVERAGE_DIR}"

    print_info "Generating HTML report..."
    llvm-cov show "${EXECUTABLE}" \
        -instr-profile="${PROFILE_DATA}" \
        -format=html \
        -output-dir="${COVERAGE_DIR}" \
        -Xdemangler c++filt \
        -show-line-counts-or-regions \
        -show-instantiations

    print_info "HTML report generated: ${COVERAGE_DIR}/index.html"

    # Generate text summary
    print_header "Coverage Summary"
    llvm-cov report "${EXECUTABLE}" \
        -instr-profile="${PROFILE_DATA}" \
        -use-color
}

function clean_coverage() {
    print_header "Cleaning Coverage Data"

    cd "${SCRIPT_DIR}"

    print_info "Removing .profraw files..."
    rm -f "${BUILD_DIR}"/*.profraw

    print_info "Removing merged profile data..."
    rm -f "${PROFILE_DATA}"

    print_info "Removing HTML report..."
    rm -rf "${COVERAGE_DIR}"

    print_info "Coverage data cleaned"
}

function show_usage() {
    cat << EOF
Code Coverage Analysis Script

Usage:
    $0 [command]

Commands:
    (no args)    - Full workflow: build → test → report
    build        - Build project with coverage instrumentation
    test         - Run tests to generate coverage data
    merge        - Merge .profraw files into .profdata
    report       - Generate HTML coverage report
    clean        - Remove all coverage data and reports
    help         - Show this help message

Examples:
    $0                  # Run full coverage analysis
    $0 build            # Just build with coverage
    $0 report           # Just generate report from existing data

Output:
    Build artifacts:    ${BUILD_DIR}/
    Coverage data:      ${PROFILE_DATA}
    HTML report:        ${COVERAGE_DIR}/index.html

Requirements:
    - Clang compiler (clang++)
    - LLVM tools (llvm-cov, llvm-profdata)
    - Qt5 development libraries

EOF
}

# Main script logic
case "${1:-full}" in
    build)
        build_with_coverage
        ;;
    test)
        run_tests
        merge_coverage_data
        ;;
    merge)
        merge_coverage_data
        ;;
    report)
        generate_report
        ;;
    clean)
        clean_coverage
        ;;
    help|--help|-h)
        show_usage
        ;;
    full)
        build_with_coverage
        run_tests
        merge_coverage_data
        generate_report
        echo ""
        print_header "Coverage Analysis Complete"
        print_info "Open coverage report: firefox ${COVERAGE_DIR}/index.html"
        ;;
    *)
        print_error "Unknown command: $1"
        show_usage
        exit 1
        ;;
esac
