#include "node_registry.h"
#include "node.h"
#include <QDebug>

NodeRegistry& NodeRegistry::instance()
{
    static NodeRegistry registry;
    return registry;
}

void NodeRegistry::registerNode(const QString& typeName, NodeFactoryFunction factory)
{
    if (m_factories.contains(typeName)) {
        qWarning() << "NodeRegistry: Overwriting existing registration for type:" << typeName;
    }
    
    m_factories[typeName] = factory;
    // qDebug() << "NodeRegistry: Registered node type:" << typeName;
}

Node* NodeRegistry::createNode(const QString& typeName) const
{
    auto it = m_factories.find(typeName);
    if (it != m_factories.end()) {
        Node* node = it.value()();
        if (node) {
            // qDebug() << "NodeRegistry: Created node of type:" << typeName;
            return node;
        } else {
            qCritical() << "NodeRegistry: Factory function returned null for type:" << typeName;
        }
    } else {
        qWarning() << "NodeRegistry: Unknown node type requested:" << typeName;
        qDebug() << "Available types:" << getRegisteredTypes();
    }
    return nullptr;
}

bool NodeRegistry::isRegistered(const QString& typeName) const
{
    return m_factories.contains(typeName);
}

QStringList NodeRegistry::getRegisteredTypes() const
{
    return m_factories.keys();
}

void NodeRegistry::clear()
{
    qDebug() << "NodeRegistry: Clearing all registrations";
    m_factories.clear();
}