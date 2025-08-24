#pragma once

#include <QString>
#include <QStringList>
#include <QMap>
#include <QVariantMap>
#include <libxml/tree.h>

/**
 * NodeTemplate - Template definition for node types
 * 
 * Core template structure defining node characteristics:
 * - Identity: id, displayName, aliases
 * - Sockets: inputs, outputs counts
 * - Optional: className, iconPath, color, defaults, constraints
 */
struct NodeTemplate {
    QString id;              // Canonical ID (stored as UPPER)
    QString displayName;     // Human-readable name
    int inputs;              // Number of input sockets
    int outputs;             // Number of output sockets
    QStringList aliases;     // Alternative names (stored as UPPER)
    
    // Optional fields (for future extension)
    QString className;       // C++ class name for typed nodes
    QString iconPath;        // Path to node icon
    QString color;           // Node color hint
    QVariantMap defaults;    // Default property values
    QVariantMap constraints; // Validation constraints
    
    NodeTemplate() : inputs(0), outputs(0) {}
    
    NodeTemplate(const QString& id, const QString& displayName, int inputs, int outputs)
        : id(id.toUpper()), displayName(displayName), inputs(inputs), outputs(outputs) {}
};

/**
 * TemplateRegistry - XML-based node template loader and lookup
 * 
 * Features:
 * - libxml2-based XML loading
 * - Case-insensitive ID and alias lookup
 * - Alias mapping (alias -> canonical ID)
 * - Duplicate detection and conflict resolution
 */
class TemplateRegistry
{
public:
    TemplateRegistry();
    ~TemplateRegistry();
    
    // Core loading and lookup
    bool loadFromXmlFile(const QString& filePath);
    const NodeTemplate* find(const QString& idOrAlias) const;
    QStringList allIds() const;
    QStringList allAliases() const;
    
    // Registry information
    int templateCount() const { return m_byId.size(); }
    bool isEmpty() const { return m_byId.isEmpty(); }
    void clear();
    
    // Validation and diagnostics
    bool hasTemplate(const QString& idOrAlias) const;
    QString resolveAlias(const QString& alias) const;
    QStringList getAliasesFor(const QString& id) const;
    
private:
    // Template storage
    QMap<QString, NodeTemplate> m_byId;        // Canonical ID (UPPER) -> Template
    QMap<QString, QString> m_aliasToId;        // Alias (UPPER) -> Canonical ID (UPPER)
    
    // XML parsing helpers
    bool parseTemplateFromXml(xmlNodePtr node, NodeTemplate& tmpl);
    void parseAliases(xmlNodePtr node, NodeTemplate& tmpl);
    void parseOptionalFields(xmlNodePtr node, NodeTemplate& tmpl);
    QString getXmlProperty(xmlNodePtr node, const QString& name) const;
    
    // Registration helpers
    void registerTemplate(const NodeTemplate& tmpl);
    bool checkForConflicts(const NodeTemplate& tmpl) const;
    void logTemplateRegistered(const NodeTemplate& tmpl) const;
};