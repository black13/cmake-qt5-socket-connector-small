#pragma once

#include <QString>
#include <QHash>
#include <QPointF>
#include <QUuid>
#include <QVariantMap>
#include <QStringList>

/**
 * NodeTypeTemplates - Scriptable XML-first node type system
 * 
 * Core Architecture:
 * - All node creation goes through XML templates (GUI, JS, AI, plugins)
 * - Templates are data-driven and runtime-extensible
 * - Built for future scriptability and AI integration
 * 
 * Scriptable Future:
 * - JavaScript can register new node types
 * - AI can generate templates from natural language
 * - Plugins distribute templates, not compiled code
 * - Community shares node definitions as data
 */
class NodeTypeTemplates
{
public:
    /**
     * Get XML template for a node type
     * Searches registered templates first, falls back to built-ins
     * @param nodeType The type identifier (e.g., "SOURCE", "SINK")
     * @return XML template string, or empty if type unknown
     */
    static QString getTemplate(const QString& nodeType);
    
    /**
     * Generate complete XML specification for node creation
     * This is the SINGLE ENTRY POINT for all node creation
     * @param nodeType The type identifier
     * @param position Scene coordinates for node placement
     * @param parameters Optional parameters for template customization
     * @param nodeId Optional UUID (generates new one if null)
     * @return Complete XML ready for GraphFactory::createNodeFromXml()
     */
    static QString generateNodeXml(const QString& nodeType, 
                                 const QPointF& position,
                                 const QVariantMap& parameters = QVariantMap(),
                                 const QUuid& nodeId = QUuid());
    
    /**
     * Register new node type template (enables scripting/plugins)
     * @param nodeType Type identifier
     * @param xmlTemplate XML template for this type
     */
    static void registerTemplate(const QString& nodeType, const QString& xmlTemplate);
    
    /**
     * Get all available node types (built-in + registered)
     * @return List of all known node type identifiers
     */
    static QStringList getAvailableTypes();
    
    /**
     * Check if a node type is registered
     * @param nodeType Type to check
     * @return True if type is available
     */
    static bool hasNodeType(const QString& nodeType);
    
    /**
     * Remove registered node type (for dynamic systems)
     * Built-in types cannot be removed
     * @param nodeType Type to unregister
     */
    static void unregisterTemplate(const QString& nodeType);
    
    /**
     * Clear all registered templates (preserves built-ins)
     * Useful for plugin reload scenarios
     */
    static void clearRegisteredTemplates();

    // Future scriptability hooks
    /**
     * Register node type from JavaScript definition (future)
     * @param jsDefinition JavaScript node definition
     * @return Generated node type identifier
     */
    static QString registerFromJavaScript(const QString& jsDefinition);
        
private:

    /**
     * Load templates from Qt resource file
     * @param resourcePath Path to resource file (e.g., ":/config/node_types.xml")
     * @param templates Hash to populate with loaded templates
     */
    static void loadFromResource(const QString& resourcePath, 
                                QHash<QString, QString>& templates);
    /**
     * Get built-in node type templates
     * These are the core node types that ship with the system
     */
    static QHash<QString, QString> getBuiltinTemplates();
    
    /**
     * Inject position, ID, and custom parameters into XML template
     * @param xmlTemplate Base template string
     * @param position Node position
     * @param nodeId Node UUID
     * @param parameters Custom parameters to inject
     * @return Complete XML with all values injected
     */
    static QString injectDynamicValues(const QString& xmlTemplate, 
                                     const QPointF& position,
                                     const QUuid& nodeId,
                                     const QVariantMap& parameters);
    
    // Runtime template storage for scriptable extensions
    static QHash<QString, QString> s_registeredTemplates;
    static bool s_initialized;
    
    static void ensureInitialized();
};