#include "node_templates.h"
#include <QDebug>
#include <QXmlStreamWriter>
#include <QXmlStreamReader>

// Static storage for runtime-registered templates
QHash<QString, QString> NodeTypeTemplates::s_registeredTemplates;
bool NodeTypeTemplates::s_initialized = false;

QString NodeTypeTemplates::getTemplate(const QString& nodeType)
{
    qDebug() << __FUNCTION__ << "- TEMPLATE SYSTEM: Getting template for node type:" << nodeType;
    ensureInitialized();
    
    // Check registered templates first (allows overriding built-ins)
    if (s_registeredTemplates.contains(nodeType)) {
        qDebug() << __FUNCTION__ << "- TEMPLATE SYSTEM: Using registered template for" << nodeType;
        return s_registeredTemplates.value(nodeType);
    }
    
    // Fall back to built-in templates
    QHash<QString, QString> builtins = getBuiltinTemplates();
    QString result = builtins.value(nodeType);
    if (!result.isEmpty()) {
        qDebug() << __FUNCTION__ << "- TEMPLATE SYSTEM: Using built-in template for" << nodeType;
    } else {
        qDebug() << __FUNCTION__ << "- TEMPLATE SYSTEM: No template found for" << nodeType;
    }
    return result;
}

QString NodeTypeTemplates::generateNodeXml(const QString& nodeType, 
                                          const QPointF& position,
                                          const QVariantMap& parameters,
                                          const QUuid& nodeId)
{
    qDebug() << __FUNCTION__ << "- TEMPLATE SYSTEM: Generating XML for" << nodeType << "at position" << position;
    
    QString xmlTemplate = getTemplate(nodeType);
    if (xmlTemplate.isEmpty()) {
        qWarning() << __FUNCTION__ << "- TEMPLATE SYSTEM: FAILED - Unknown node type:" << nodeType;
        return QString();
    }
    
    // Generate UUID if not provided
    QUuid actualId = nodeId.isNull() ? QUuid::createUuid() : nodeId;
    qDebug() << __FUNCTION__ << "- TEMPLATE SYSTEM: Using UUID" << actualId.toString(QUuid::WithoutBraces);
    
    // Inject dynamic values into template
    QString result = injectDynamicValues(xmlTemplate, position, actualId, parameters);
    qDebug() << __FUNCTION__ << "- TEMPLATE SYSTEM: Generated XML node successfully";
    return result;
}

void NodeTypeTemplates::registerTemplate(const QString& nodeType, const QString& xmlTemplate)
{
    ensureInitialized();
    
    if (xmlTemplate.isEmpty()) {
        qWarning() << "NodeTypeTemplates::registerTemplate - Empty template for type:" << nodeType;
        return;
    }
    
    s_registeredTemplates[nodeType] = xmlTemplate;
    qDebug() << "NodeTypeTemplates: Registered template for type:" << nodeType;
}

QStringList NodeTypeTemplates::getAvailableTypes()
{
    ensureInitialized();
    
    QStringList types;
    
    // Add built-in types
    QHash<QString, QString> builtins = getBuiltinTemplates();
    types.append(builtins.keys());
    
    // Add registered types (may override built-ins, that's ok)
    types.append(s_registeredTemplates.keys());
    
    // Remove duplicates and sort
    types.removeDuplicates();
    types.sort();
    
    return types;
}

bool NodeTypeTemplates::hasNodeType(const QString& nodeType)
{
    ensureInitialized();
    
    if (s_registeredTemplates.contains(nodeType)) {
        return true;
    }
    
    QHash<QString, QString> builtins = getBuiltinTemplates();
    return builtins.contains(nodeType);
}

void NodeTypeTemplates::unregisterTemplate(const QString& nodeType)
{
    ensureInitialized();
    
    if (s_registeredTemplates.remove(nodeType) > 0) {
        qDebug() << "NodeTypeTemplates: Unregistered template for type:" << nodeType;
    } else {
        qWarning() << "NodeTypeTemplates::unregisterTemplate - Type not found:" << nodeType;
    }
}

void NodeTypeTemplates::clearRegisteredTemplates()
{
    ensureInitialized();
    
    int count = s_registeredTemplates.size();
    s_registeredTemplates.clear();
    qDebug() << "NodeTypeTemplates: Cleared" << count << "registered templates";
}

QString NodeTypeTemplates::registerFromJavaScript(const QString& jsDefinition)
{
    // Future implementation - placeholder for scriptable system
    Q_UNUSED(jsDefinition);
    qDebug() << "NodeTypeTemplates::registerFromJavaScript - Future feature placeholder";
    return QString();
}

int NodeTypeTemplates::loadFromFile(const QString& templateFilePath)
{
    // Future implementation - placeholder for plugin system
    Q_UNUSED(templateFilePath);
    qDebug() << "NodeTypeTemplates::loadFromFile - Future feature placeholder";
    return 0;
}

QHash<QString, QString> NodeTypeTemplates::getBuiltinTemplates()
{
    static QHash<QString, QString> templates = {
        // Core node types with socket configurations
        {"SOURCE", R"(<node type="SOURCE" inputs="0" outputs="1"/>)"},
        {"SINK",   R"(<node type="SINK" inputs="1" outputs="0"/>)"},
        {"SPLIT",  R"(<node type="SPLIT" inputs="1" outputs="2"/>)"},
        {"MERGE",  R"(<node type="MERGE" inputs="2" outputs="1"/>)"},
        {"TRANSFORM", R"(<node type="TRANSFORM" inputs="1" outputs="1"/>)"}
    };
    
    return templates;
}

QString NodeTypeTemplates::injectDynamicValues(const QString& xmlTemplate, 
                                              const QPointF& position,
                                              const QUuid& nodeId,
                                              const QVariantMap& parameters)
{
    QString result = xmlTemplate;
    
    // Inject node ID
    QString idString = nodeId.toString(QUuid::WithoutBraces);
    result.replace(QStringLiteral("{{ID}}"), idString);
    if (!result.contains(QStringLiteral("id="))) {
        // If template doesn't have id attribute, add it
        result.replace(QStringLiteral("<node"), 
                      QString(QStringLiteral("<node id=\"%1\"")).arg(idString));
    }
    
    // Inject position
    result.replace(QStringLiteral("{{X}}"), QString::number(position.x()));
    result.replace(QStringLiteral("{{Y}}"), QString::number(position.y()));
    if (!result.contains(QStringLiteral("x="))) {
        // If template doesn't have position attributes, add them
        QString withPos = result;
        withPos.replace(QStringLiteral("<node"), 
                       QString(QStringLiteral("<node x=\"%1\" y=\"%2\""))
                       .arg(position.x()).arg(position.y()));
        result = withPos;
    }
    
    // Inject custom parameters (future extensibility)
    for (auto it = parameters.constBegin(); it != parameters.constEnd(); ++it) {
        QString placeholder = QString("{{%1}}").arg(it.key().toUpper());
        result.replace(placeholder, it.value().toString());
    }
    
    qDebug() << "NodeTypeTemplates: Generated XML:" << result;
    return result;
}

void NodeTypeTemplates::ensureInitialized()
{
    if (!s_initialized) {
        // Future: Load templates from config files, plugins, etc.
        qDebug() << "NodeTypeTemplates: System initialized with" 
                 << getBuiltinTemplates().size() << "built-in templates";
        s_initialized = true;
    }
}