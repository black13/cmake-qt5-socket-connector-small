#!/bin/bash
# CI Guardrails - Enforce architectural rules
# Prevents regression to anti-patterns

set -e

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$REPO_ROOT"

EXIT_CODE=0

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "CI Lint Guardrails - Enforcing Architectural Rules"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

# Rule 1: No embedded JavaScript in C++ (use external .js files)
# Exception: node_templates.cpp can have R"( for XML templates
echo ""
echo "[Rule 1] Ban embedded raw string literals R\"( for JavaScript"
EMBEDDED_JS=$(grep -rn 'R"(' --include='*.cpp' --include='*.h' . \
    --exclude-dir=commits \
    --exclude-dir=build_linux \
    --exclude-dir=coverage_html \
    | grep -v 'node_templates.cpp' || true)
if [ -n "$EMBEDDED_JS" ]; then
    echo "❌ FAIL: Found embedded raw string literals (use external .js files)"
    echo "$EMBEDDED_JS"
    EXIT_CODE=1
else
    echo "✅ PASS: No embedded raw string literals found (node_templates.cpp XML templates allowed)"
fi

# Rule 2: No qgraphicsitem_cast in application code (use metadata keys)
echo ""
echo "[Rule 2] Ban qgraphicsitem_cast< (use Gik::KindKey metadata)"
CASTS=$(grep -rn 'qgraphicsitem_cast<' --include='*.cpp' --include='*.h' . \
    --exclude-dir=commits \
    --exclude-dir=build_linux \
    --exclude-dir=coverage_html \
    || true)
if [ -n "$CASTS" ]; then
    echo "❌ FAIL: Found qgraphicsitem_cast (use metadata keys instead)"
    echo "$CASTS"
    EXIT_CODE=1
else
    echo "✅ PASS: No qgraphicsitem_cast found"
fi

# Rule 3: No type() logic (use Gik::KindKey metadata)
echo ""
echo "[Rule 3] Ban item->type() == Node::Type logic (use metadata)"
TYPE_LOGIC=$(grep -rn '->type().*==' --include='*.cpp' --include='*.h' . \
    --exclude-dir=commits \
    --exclude-dir=build_linux \
    --exclude-dir=coverage_html \
    || true)
if [ -n "$TYPE_LOGIC" ]; then
    echo "❌ FAIL: Found type() comparison logic (use metadata keys instead)"
    echo "$TYPE_LOGIC"
    EXIT_CODE=1
else
    echo "✅ PASS: No type() comparison logic found"
fi

echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
if [ $EXIT_CODE -eq 0 ]; then
    echo "✅ All architectural rules passed!"
else
    echo "❌ Some architectural rules failed - review violations above"
fi
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

exit $EXIT_CODE
