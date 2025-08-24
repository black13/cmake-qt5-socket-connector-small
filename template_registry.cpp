#include "template_registry.h"
#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <libxml/parser.h>
#include <libxml/tree.h>

TemplateRegistry::TemplateRegistry()
{
    // Initialize libxml2
    xmlInitParser();
}

TemplateRegistry::~TemplateRegistry()
{
    clear();
    // Cleanup libxml2 (called once at program end)
    xmlCleanupParser();
}

bool TemplateRegistry::loadFromXmlFile(const QString& filePath)
{
    if (filePath.isEmpty()) {
        qWarning() << "TemplateRegistry: Empty file path";
        return false;
    }
    
    QFile file(filePath);
    if (!file.exists()) {
        qWarning() << "TemplateRegistry: Template file not found:" << filePath;
        return false;
    }
    
    qDebug() << "TemplateRegistry: Loading templates from:" << filePath;
    
    // Parse XML document
    xmlDocPtr doc = xmlParseFile(filePath.toUtf8().constData());
    if (!doc) {
        qCritical() << "TemplateRegistry: Failed to parse XML file:" << filePath;
        return false;
    }
    
    xmlNodePtr root = xmlDocGetRootElement(doc);
    if (!root) {
        qCritical() << "TemplateRegistry: No root element in XML file:" << filePath;
        xmlFreeDoc(doc);
        return false;
    }
    
    // Expect <node_templates> root element
    if (xmlStrcmp(root->name, BAD_CAST "node_templates") != 0) {
        qWarning() << "TemplateRegistry: Expected <node_templates> root element, got:" 
                   << (char*)root->name;
    }
    
    int templatesLoaded = 0;
    int templatesSkipped = 0;
    
    // Parse each <template> element
    for (xmlNodePtr node = root->children; node; node = node->next) {
        if (node->type != XML_ELEMENT_NODE) continue;
        
        if (xmlStrcmp(node->name, BAD_CAST "template") == 0) {
            NodeTemplate tmpl;
            if (parseTemplateFromXml(node, tmpl)) {
                if (checkForConflicts(tmpl)) {
                    qWarning() << "TemplateRegistry: Skipping template due to conflicts:" << tmpl.id;
                    templatesSkipped++;
                } else {
                    registerTemplate(tmpl);
                    logTemplateRegistered(tmpl);
                    templatesLoaded++;
                }
            } else {
                qWarning() << "TemplateRegistry: Failed to parse template element";
                templatesSkipped++;
            }
        }
    }
    
    xmlFreeDoc(doc);
    
    qDebug() << "TemplateRegistry: Loaded" << templatesLoaded << "templates," 
             << templatesSkipped << "skipped from" << filePath;
    
    return templatesLoaded > 0;
}

const NodeTemplate* TemplateRegistry::find(const QString& idOrAlias) const
{
    if (idOrAlias.isEmpty()) return nullptr;
    
    QString upperKey = idOrAlias.toUpper();
    
    // Direct ID lookup
    auto it = m_byId.find(upperKey);
    if (it != m_byId.end()) {
        return &it.value();
    }
    
    // Alias lookup
    auto aliasIt = m_aliasToId.find(upperKey);
    if (aliasIt != m_aliasToId.end()) {
        QString canonicalId = aliasIt.value();
        auto templateIt = m_byId.find(canonicalId);
        if (templateIt != m_byId.end()) {
            return &templateIt.value();
        }
    }
    
    return nullptr;
}

QStringList TemplateRegistry::allIds() const
{
    return m_byId.keys();
}

QStringList TemplateRegistry::allAliases() const
{
    return m_aliasToId.keys();
}

void TemplateRegistry::clear()
{
    m_byId.clear();
    m_aliasToId.clear();
}

bool TemplateRegistry::hasTemplate(const QString& idOrAlias) const
{
    return find(idOrAlias) != nullptr;
}

QString TemplateRegistry::resolveAlias(const QString& alias) const
{
    QString upperAlias = alias.toUpper();
    return m_aliasToId.value(upperAlias, QString());
}

QStringList TemplateRegistry::getAliasesFor(const QString& id) const
{
    QString upperId = id.toUpper();
    QStringList aliases;
    
    for (auto it = m_aliasToId.begin(); it != m_aliasToId.end(); ++it) {
        if (it.value() == upperId) {
            aliases << it.key();
        }
    }
    
    return aliases;
}

bool TemplateRegistry::parseTemplateFromXml(xmlNodePtr node, NodeTemplate& tmpl)
{
    if (!node) return false;
    
    // Required attributes
    QString id = getXmlProperty(node, "id");
    QString displayName = getXmlProperty(node, "displayName");
    QString inputsStr = getXmlProperty(node, "inputs");
    QString outputsStr = getXmlProperty(node, "outputs");
    
    if (id.isEmpty()) {
        qWarning() << "TemplateRegistry: Template missing required 'id' attribute";
        return false;
    }
    
    // Set core fields
    tmpl.id = id.toUpper();
    tmpl.displayName = displayName.isEmpty() ? id : displayName;
    tmpl.inputs = inputsStr.toInt();
    tmpl.outputs = outputsStr.toInt();
    
    // Parse aliases
    parseAliases(node, tmpl);
    
    // Parse optional fields
    parseOptionalFields(node, tmpl);
    
    return true;
}

void TemplateRegistry::parseAliases(xmlNodePtr node, NodeTemplate& tmpl)
{
    QString aliasesStr = getXmlProperty(node, "aliases");
    if (!aliasesStr.isEmpty()) {
        QStringList aliases = aliasesStr.split(",", Qt::SkipEmptyParts);
        for (QString alias : aliases) {
            alias = alias.trimmed().toUpper();
            if (!alias.isEmpty()) {
                tmpl.aliases << alias;
            }
        }
    }
}

void TemplateRegistry::parseOptionalFields(xmlNodePtr node, NodeTemplate& tmpl)
{
    tmpl.className = getXmlProperty(node, "className");
    tmpl.iconPath = getXmlProperty(node, "iconPath");
    tmpl.color = getXmlProperty(node, "color");
    
    // TODO: Parse defaults and constraints if needed
}

QString TemplateRegistry::getXmlProperty(xmlNodePtr node, const QString& name) const
{
    if (!node) return QString();
    
    xmlChar* prop = xmlGetProp(node, BAD_CAST name.toUtf8().constData());
    if (!prop) return QString();
    
    QString result = QString::fromUtf8((char*)prop);
    xmlFree(prop);
    return result;
}

void TemplateRegistry::registerTemplate(const NodeTemplate& tmpl)
{
    // Register main ID
    m_byId[tmpl.id] = tmpl;
    
    // Register aliases
    for (const QString& alias : tmpl.aliases) {
        m_aliasToId[alias] = tmpl.id;
    }
}

bool TemplateRegistry::checkForConflicts(const NodeTemplate& tmpl) const
{
    // Check ID conflict
    if (m_byId.contains(tmpl.id)) {
        qWarning() << "TemplateRegistry: Duplicate template ID:" << tmpl.id;
        return true;
    }
    
    // Check alias conflicts
    for (const QString& alias : tmpl.aliases) {
        if (m_aliasToId.contains(alias)) {
            qWarning() << "TemplateRegistry: Duplicate alias:" << alias 
                       << "for template:" << tmpl.id 
                       << "(conflicts with:" << m_aliasToId[alias] << ")";
            return true;
        }
        
        // Check if alias conflicts with existing ID
        if (m_byId.contains(alias)) {
            qWarning() << "TemplateRegistry: Alias conflicts with existing ID:" << alias;
            return true;
        }
    }
    
    return false;
}

void TemplateRegistry::logTemplateRegistered(const NodeTemplate& tmpl) const
{
    QString aliasesStr = tmpl.aliases.isEmpty() ? "none" : tmpl.aliases.join(", ");
    qDebug() << "TemplateRegistry: Registered template:"
             << "id=" << tmpl.id
             << "display=" << tmpl.displayName
             << "sockets=" << tmpl.inputs << "in," << tmpl.outputs << "out"
             << "aliases=[" << aliasesStr << "]";
}