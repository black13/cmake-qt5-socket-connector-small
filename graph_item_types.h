#pragma once

#include <QString>

/**
 * GraphItemType - Explicit type system for graph items
 * 
 * NO MORE qgraphicsitem_cast! Use explicit type checking instead.
 * This is safer, faster, and JavaScript-friendly.
 */
enum class GraphItemType {
    Node,
    Edge, 
    Socket,
    GhostEdge,
    Unknown
};

/**
 * Convert type enum to string (useful for JavaScript and debugging)
 */
inline QString graphItemTypeToString(GraphItemType type) {
    switch (type) {
        case GraphItemType::Node: return "NODE";
        case GraphItemType::Edge: return "EDGE"; 
        case GraphItemType::Socket: return "SOCKET";
        case GraphItemType::GhostEdge: return "GHOST_EDGE";
        case GraphItemType::Unknown: return "UNKNOWN";
    }
    return "INVALID";
}

/**
 * GraphItem - Base interface for all graph items
 * 
 * Every graph item MUST implement these methods for safe type checking
 */
class IGraphItem {
public:
    virtual ~IGraphItem() = default;
    virtual GraphItemType getGraphItemType() const = 0;
    virtual QString getItemId() const = 0;
    virtual QString getItemTypeString() const { 
        return graphItemTypeToString(getGraphItemType()); 
    }
};