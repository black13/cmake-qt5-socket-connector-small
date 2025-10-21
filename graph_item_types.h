#pragma once

#include <QString>

/**
 * Type enumeration for graph items
 * Used for type-safe casting and identification
 */
enum class GraphItemType {
    Node,
    Edge,
    Socket,
    GhostEdge
};

/**
 * Interface for all graph items
 * Provides common identification and type system
 */
class IGraphItem {
public:
    virtual ~IGraphItem() = default;
    
    // Type identification
    virtual GraphItemType getGraphItemType() const = 0;
    
    // String-based item identification
    virtual QString getItemId() const = 0;
};