#pragma once

#include <QtGlobal>

/**
 * Centralized layout metrics for the node graph system
 *
 * All geometric constants used for node/socket/edge layout in one place.
 * This prevents duplication, ensures consistency, and makes global adjustments easy.
 *
 * Usage: Include this header and reference LayoutMetrics::socketRadius, etc.
 */
namespace LayoutMetrics {

    // Socket geometry
    constexpr qreal socketRadius = 8.0;         // Socket circle radius
    constexpr qreal socketDiameter = socketRadius * 2.0;  // Derived: 16.0
    constexpr qreal socketSpacing = 32.0;       // Vertical spacing between sockets

    // Node padding (space for sockets and labels)
    constexpr qreal nodePaddingTop = 14.0;      // Top padding (socket clearance)
    constexpr qreal nodePaddingBottom = 14.0;   // Bottom padding (socket clearance)
    constexpr qreal socketOffsetFromEdge = 4.0; // Horizontal offset of socket centers from node edge

    // Node minimum dimensions
    constexpr qreal minNodeWidth = 100.0;       // Minimum width for readable node labels
    constexpr qreal minNodeHeight = 50.0;       // Minimum height for node structure

    // Edge update optimization
    constexpr qreal edgeUpdateThreshold = 5.0;  // Manhattan pixels before updating connected edges

    // Derived measurements (computed from above constants)
    constexpr qreal socketCenterY(int index) {
        return nodePaddingTop + index * socketSpacing;
    }

    constexpr qreal minHeightForSockets(int socketCount) {
        if (socketCount == 0) return minNodeHeight;
        return nodePaddingTop + (socketCount - 1) * socketSpacing + socketDiameter + nodePaddingBottom;
    }

} // namespace LayoutMetrics
