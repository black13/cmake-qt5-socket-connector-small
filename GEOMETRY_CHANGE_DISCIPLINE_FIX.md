# Geometry Change Discipline Fix

**Branch:** `fix/geometry-change-discipline`
**Date:** 2025-10-04
**Issue:** Qt BSP cache corruption from improper geometry mutation order

---

## Problem Statement

Qt's graphics framework uses a BSP (Binary Space Partitioning) spatial index to efficiently manage scene items. When an item's geometry changes, Qt must update this index. The contract is:

**RULE:** Call `prepareGeometryChange()` BEFORE mutating any geometry data.

Violating this causes:
- BSP index corruption (stale bounds cached)
- Visual glitches (item not repainted correctly)
- Picker failures (item not found at correct position)
- Performance degradation (unnecessary scene scans)

---

## Violations Found

### Violation 1: Node::calculateNodeSize()

**File:** `node.cpp:525-560`
**Issue:** Called `prepareGeometryChange()` AFTER modifying `m_width` and `m_height`

```cpp
// ❌ WRONG ORDER (before fix)
void Node::calculateNodeSize(int inputCount, int outputCount)
{
    // Calculate dimensions
    m_width = qMax(minNodeWidth, textWidth);   // ← Mutation #1
    m_height = qMax(minNodeHeight, requiredHeight);  // ← Mutation #2

    // Notify Qt graphics system of geometry change
    prepareGeometryChange();  // ← TOO LATE! BSP already stale
}
```

**Why this is wrong:**
- `m_width` and `m_height` affect `boundingRect()`
- BSP index uses `boundingRect()` for spatial queries
- By the time we call `prepareGeometryChange()`, BSP already cached old bounds
- Next repaint uses stale bounds, causing visual artifacts

**Fix:**
```cpp
// ✅ CORRECT ORDER (after fix)
void Node::calculateNodeSize(int inputCount, int outputCount)
{
    // ✅ CRITICAL: Notify Qt BSP cache BEFORE changing geometry
    prepareGeometryChange();

    // NOW safe to mutate
    m_width = qMax(minNodeWidth, textWidth);
    m_height = qMax(minNodeHeight, requiredHeight);
}
```

---

### Violation 2: Edge::buildPath()

**File:** `edge.cpp:260-334`
**Issue:** Called `m_path.clear()` without `prepareGeometryChange()` first

```cpp
// ❌ WRONG ORDER (before fix)
void Edge::buildPath(const QPointF& start, const QPointF& end)
{
    // Clear and rebuild path safely
    m_path.clear();  // ← MUTATION WITHOUT NOTIFICATION!

    // ... build new path ...
    m_path.moveTo(adjustedStart);
    m_path.cubicTo(control1, control2, adjustedEnd);

    // Notify Qt's BSP cache before changing bounding rectangle
    prepareGeometryChange();  // ← Called here, but path already changed!

    m_boundingRect = pathBounds.adjusted(-10, -10, 10, 10);
}
```

**Why this is wrong:**
- `m_path` affects `boundingRect()` via `m_path.boundingRect()`
- Calling `clear()` changes the path geometry immediately
- BSP index not notified of this change
- Later `prepareGeometryChange()` only covers bounding rect mutation, not path

**Fix:**
```cpp
// ✅ CORRECT ORDER (after fix)
void Edge::buildPath(const QPointF& start, const QPointF& end)
{
    // ✅ CRITICAL: Notify Qt BSP cache BEFORE modifying path
    prepareGeometryChange();

    // NOW safe to mutate path
    m_path.clear();
    m_path.moveTo(adjustedStart);
    m_path.cubicTo(control1, control2, adjustedEnd);

    // Update bounding rectangle
    // (already notified above, covers all mutations)
    m_boundingRect = pathBounds.adjusted(-10, -10, 10, 10);
}
```

**Bonus optimization:**
- Single `prepareGeometryChange()` at function start covers ALL mutations
- Removed redundant second call before `m_boundingRect` assignment
- More efficient (one BSP update instead of two)

---

## Impact

### Before Fix
1. **Node resizing:** BSP index stale during socket creation → visual glitches when nodes change size
2. **Edge path updates:** Path mutations bypass spatial index → picker fails to find edges at new positions
3. **Redundant calls:** Edge had 2 `prepareGeometryChange()` calls when 1 suffices → wasted CPU

### After Fix
1. **Node resizing:** BSP index updated before mutations → smooth visual updates
2. **Edge path updates:** All geometry changes properly notified → picker works correctly
3. **Optimized:** Single notification per function → better performance

---

## Testing

**Manual verification:**
1. Create nodes with different socket counts → resize works smoothly
2. Drag nodes around → connected edges update without flicker
3. Click near edge curves → selection works at exact click position

**Code verification:**
```bash
# Search for geometry mutations without prepareGeometryChange()
grep -n "m_path\s*=" edge.cpp      # All preceded by prepareGeometryChange()
grep -n "m_boundingRect\s*=" *.cpp # All preceded by prepareGeometryChange()
grep -n "m_width\s*=\|m_height\s*=" node.cpp  # All preceded by prepareGeometryChange()
```

---

## Pattern to Follow

**Always use this pattern for QGraphicsItem geometry:**

```cpp
void MyItem::changeGeometry()
{
    // 1. Notify FIRST
    prepareGeometryChange();

    // 2. Mutate geometry data
    m_boundingRect = newRect;
    m_path = newPath;
    m_width = newWidth;
    // ... any other geometry fields ...

    // 3. Optional: trigger repaint
    update();
}
```

**What counts as "geometry"?**
- Anything that affects `boundingRect()` return value
- `m_path`, `m_boundingRect`, `m_width`, `m_height`, position, scale, rotation
- Even if cached separately, if it influences bounds, notify first

---

## References

- Qt Documentation: [QGraphicsItem::prepareGeometryChange()](https://doc.qt.io/qt-5/qgraphicsitem.html#prepareGeometryChange)
- Code locations:
  - `node.cpp:527` - Fixed calculateNodeSize()
  - `edge.cpp:274` - Fixed buildPath()
  - `ghost_edge.cpp:18` - Already correct ✅

---

## Files Changed

- `edge.cpp`: Moved prepareGeometryChange() call, removed duplicate
- `node.cpp`: Moved prepareGeometryChange() to function start

**Diff summary:**
- 2 files changed
- 3 lines moved (not added/removed)
- Net effect: More correct, more efficient
