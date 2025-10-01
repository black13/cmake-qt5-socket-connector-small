#!/bin/bash
# CI Check: Forbidden Patterns
# Fails the build if code contains anti-patterns identified in git history analysis
#
# Patterns checked:
# 1. R"( in .cpp files (embedded JavaScript strings)
# 2. qgraphicsitem_cast in application code (not library/engine files)
#
# Exit code: 0 = pass, 1 = fail

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "🔍 Forbidden Pattern Check (Anti-Backsliding)"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

FAIL=0

# ==============================================================================
# Check 1: R"( in .cpp files (embedded JavaScript)
# ==============================================================================

echo "📋 Check 1: Embedded JavaScript strings (R\"(...) pattern)"
echo "   Rule: JavaScript should be in external .js files, not embedded in C++"
echo ""

# Exclude javascript_engine.cpp - it's the API binding layer (library code)
EMBEDDED_JS=$(grep -n 'R"(' *.cpp 2>/dev/null | grep -v javascript_engine.cpp | grep -v '//' || true)

if [ -n "$EMBEDDED_JS" ]; then
    echo -e "${RED}❌ FAIL: Found embedded JavaScript strings in application code${NC}"
    echo ""
    echo "Violations:"
    echo "$EMBEDDED_JS"
    echo ""
    echo "Fix: Move JavaScript code to external .js files in scripts/ directory"
    echo "     Use Graph.evaluateFile() or loadAndExecuteScript() instead"
    echo ""
    FAIL=1
else
    echo -e "${GREEN}✅ PASS: No embedded JavaScript strings found${NC}"
    echo ""
fi

# ==============================================================================
# Check 2: qgraphicsitem_cast in application code
# ==============================================================================

echo "📋 Check 2: Casting in application code (qgraphicsitem_cast pattern)"
echo "   Rule: Use metadata keys (Gik::KindKey) instead of casting"
echo ""

# Casting is allowed for:
# 1. Socket* from childItems() - parent-child traversal
# 2. Socket* from scene items - ghost edge target detection
# 3. Node* from parentItem() - accessing socket's parent node
#
# Casting is FORBIDDEN for:
# - Node*/Edge* in selection logic (updateStatusBar, removeSelected, etc.)
# - Iteration over scene->items() to find nodes/edges
ALLOWED_FILES=(
    "socket.cpp"           # getParentNode() casts to Node*
    "node.cpp"             # childItems() traversal for sockets
    "scene.cpp.*Socket"    # Socket detection for ghost edges
    "graph_factory.cpp"    # childItems() traversal for sockets
    "graph_controller.cpp.*Socket"  # Socket finding for JS API
)

# Find all casts
ALL_CASTS=$(grep -n 'qgraphicsitem_cast' *.cpp 2>/dev/null || true)

if [ -z "$ALL_CASTS" ]; then
    echo -e "${GREEN}✅ PASS: No casting found${NC}"
    echo ""
else
    # Filter out allowed casts
    FORBIDDEN_CASTS=""

    while IFS= read -r line; do
        ALLOWED=0

        # Check allow list first
        for pattern in "${ALLOWED_FILES[@]}"; do
            if echo "$line" | grep -q "$pattern"; then
                ALLOWED=1
                break
            fi
        done

        # If already allowed, skip
        if [ $ALLOWED -eq 1 ]; then
            continue
        fi

        # Check for Node* or Edge* casts in non-allowed files (forbidden)
        if echo "$line" | grep -qE 'qgraphicsitem_cast<(Node|Edge)\*>'; then
            echo -e "${RED}FORBIDDEN (Node/Edge cast in selection logic):${NC} $line"
            FORBIDDEN_CASTS="$FORBIDDEN_CASTS\n$line"
        elif [ -n "$line" ]; then
            # Other casts (probably Socket*) not in allow list
            FORBIDDEN_CASTS="$FORBIDDEN_CASTS\n$line"
        fi
    done <<< "$ALL_CASTS"

    if [ -n "$FORBIDDEN_CASTS" ]; then
        echo -e "${RED}❌ FAIL: Found forbidden casts in application code${NC}"
        echo ""
        echo "Violations:"
        echo -e "$FORBIDDEN_CASTS"
        echo ""
        echo "Fix: Use metadata keys instead:"
        echo "     const auto k = item->data(Gik::KindKey);"
        echo "     if (k.toInt() == Gik::Kind_Node) { /* ... */ }"
        echo ""
        FAIL=1
    else
        echo -e "${GREEN}✅ PASS: All casts are in allowed library code (Socket access)${NC}"
        echo ""
    fi
fi

# ==============================================================================
# Summary
# ==============================================================================

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

if [ $FAIL -eq 0 ]; then
    echo -e "${GREEN}✅ ALL CHECKS PASSED${NC}"
    echo "   No anti-patterns detected"
    echo "   Code follows cast-free architecture"
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    exit 0
else
    echo -e "${RED}❌ CHECKS FAILED${NC}"
    echo "   Anti-patterns detected - see violations above"
    echo "   Review ANALYSIS_JS_EMBEDDING_HISTORY.md for context"
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    exit 1
fi
