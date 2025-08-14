#include "rubber_node_facade.h"
#include "node.h"
#include <stdexcept>
#include <QDebug>

/**
 * RubberNodeFacade Implementation - Week 1 Minimal Wrapper
 * 
 * Safety-first approach: All methods delegate to existing proven Node methods
 */

RubberNodeFacade::RubberNodeFacade(Node& node)
    : m_node(&node)
{
    // Validate node pointer immediately
    ensureValidNode();
}

// === Core Node Properties (delegate to existing Node methods) ===

QUuid RubberNodeFacade::getId() const
{
    ensureValidNode();
    return m_node->getId();
}

QString RubberNodeFacade::getType() const
{
    ensureValidNode();
    return m_node->getNodeType();
}

QPointF RubberNodeFacade::getPosition() const
{
    ensureValidNode();
    return m_node->pos();
}

void RubberNodeFacade::setPosition(const QPointF& position)
{
    ensureValidNode();
    m_node->setPos(position);
}

// === Action System (Week 2-3 placeholders) ===

void RubberNodeFacade::registerAction(const QString& name, const QString& script)
{
    ensureValidNode();
    
    // Week 1: Just store the action - no execution yet
    m_actions[name] = script;
    
    qDebug() << "RubberNodeFacade: Registered action" << name 
             << "for node" << getId().toString().left(8)
             << "type" << getType();
}

bool RubberNodeFacade::hasAction(const QString& name) const
{
    return m_actions.contains(name);
}

QHash<QString, QString> RubberNodeFacade::getActions() const
{
    return m_actions;
}

void RubberNodeFacade::removeAction(const QString& name)
{
    if (m_actions.remove(name) > 0) {
        qDebug() << "RubberNodeFacade: Removed action" << name 
                 << "from node" << getId().toString().left(8);
    }
}

// === Debugging and Validation ===

bool RubberNodeFacade::isValid() const
{
    return m_node != nullptr;
}

Node* RubberNodeFacade::getNode() const
{
    ensureValidNode();
    return m_node;
}

QString RubberNodeFacade::toString() const
{
    if (!isValid()) {
        return QString("RubberNodeFacade(INVALID)");
    }
    
    return QString("RubberNodeFacade(id=%1, type=%2, pos=%3,%4, actions=%5)")
        .arg(getId().toString().left(8))
        .arg(getType())
        .arg(getPosition().x())
        .arg(getPosition().y())
        .arg(m_actions.size());
}

// === Private Methods ===

void RubberNodeFacade::ensureValidNode() const
{
    if (!m_node) {
        throw std::runtime_error("RubberNodeFacade: Wrapped node pointer is null");
    }
}

// === Global Operators ===

bool operator==(const RubberNodeFacade& lhs, const RubberNodeFacade& rhs)
{
    // Two facades are equal if they wrap the same node
    return lhs.isValid() && rhs.isValid() && 
           (lhs.getNode() == rhs.getNode());
}

bool operator!=(const RubberNodeFacade& lhs, const RubberNodeFacade& rhs)
{
    return !(lhs == rhs);
}