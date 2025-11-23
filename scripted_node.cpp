#include "scripted_node.h"

#include <QDebug>
#include <QFont>
#include <QJsonDocument>
#include <QObject>
#include <QPainter>
#include <QPen>
#include "synthetic_work.h"
#include "socket.h"
#include <QStyleOptionGraphicsItem>
#include <libxml/tree.h>

// QObject wrapper exposed to JavaScript so scripts interact with C++ safely.
// Every method here is intentional: keep the surface area tight so scripts
// can read/write payloads, log, run synthetic workloads, and inspect basic
// metadata (id/type/sockets/edges) without poking raw C++ pointers.
class ScriptNodeApi : public QObject
{
    Q_OBJECT
public:
    explicit ScriptNodeApi(ScriptedNode* node, QObject* parent = nullptr)
        : QObject(parent)
        , m_node(node)
    {
    }

public slots:
    QVariant payloadValue(const QString& key) const
    {
        if (!m_node) {
            return {};
        }
        return m_node->payload().value(key);
    }

    void setPayloadValue(const QString& key, const QVariant& value)
    {
        if (!m_node) {
            return;
        }
        QVariantMap payload = m_node->payload();
        payload.insert(key, value);
        m_node->setPayload(payload);
    }

    QVariantMap payload() const
    {
        return m_node ? m_node->payload() : QVariantMap();
    }

    void setPayload(const QVariantMap& payload)
    {
        if (m_node) {
            m_node->setPayload(payload);
        }
    }

    void setLabel(const QString& text)
    {
        if (m_node) {
            m_node->setDisplayLabel(text);
        }
    }

    void log(const QString& message)
    {
        qDebug().noquote() << "[ScriptNode]" << message;
    }

    QVariant runWork(const QVariantMap& request)
    {
        return SyntheticWork::run(request);
    }

    QString nodeId() const
    {
        if (!m_node) {
            return {};
        }
        return m_node->getId().toString(QUuid::WithoutBraces);
    }

    QString nodeType() const
    {
        if (!m_node) {
            return {};
        }
        return m_node->getNodeType();
    }

    int socketCount(int role) const
    {
        if (!m_node) {
            return 0;
        }
        const Socket::Role socketRole = (role == Socket::Input) ? Socket::Input : Socket::Output;
        if (socketRole == Socket::Input) {
            return m_node->getInputSockets().size();
        }
        return m_node->getOutputSockets().size();
    }

    int edgeCount() const
    {
        if (!m_node) {
            return 0;
        }
        return m_node->getIncidentEdges().size();
    }

    QVariantMap info() const
    {
        QVariantMap map;
        if (!m_node) {
            return map;
        }
        map.insert(QStringLiteral("id"), nodeId());
        map.insert(QStringLiteral("type"), nodeType());
        map.insert(QStringLiteral("inputCount"), m_node->getInputSockets().size());
        map.insert(QStringLiteral("outputCount"), m_node->getOutputSockets().size());
        map.insert(QStringLiteral("edgeCount"), m_node->getIncidentEdges().size());
        return map;
    }

private:
    ScriptedNode* m_node;
};

QJSEngine* ScriptedNode::s_engine = nullptr;

ScriptedNode::ScriptedNode(const QUuid& id, const QPointF& position)
    : Node(id, position)
{
    setNodeType(QStringLiteral("SCRIPT"));
}

void ScriptedNode::setSharedEngine(QJSEngine* engine)
{
    s_engine = engine;
}

void ScriptedNode::setScript(const QString& code)
{
    m_script = code;
    m_compiledFunction = QJSValue();
    compileIfNeeded();
}

void ScriptedNode::setPayload(const QVariantMap& payload)
{
    m_payload = payload;
}

/**
 * @brief Run the node's JavaScript snippet synchronously.
 *
 * Invoked via Graph::executeNodeScript (context menu, CLI scripts, autosave
 * validations). The work happens on the UI thread, so scripts must be quick;
 * long-running operations will block the UI until the script returns.
 *
 * @param context Key/value map passed from the caller (currently unused).
 * @return QVariant result provided by the script (cached in m_lastResult).
 */
QVariant ScriptedNode::evaluate(const QVariantMap& context)
{
    QJSValue result = callScript(context);
    if (result.isError()) {
        qWarning() << "ScriptedNode: script error in"
                   << getId().toString(QUuid::WithoutBraces)
                   << result.property("message").toString();
        m_lastResult.clear();
        return {};
    }

    m_lastResult = result.toVariant();
    return m_lastResult;
}

void ScriptedNode::setDisplayLabel(const QString& text)
{
    if (m_displayLabel == text) {
        return;
    }
    m_displayLabel = text;
    update();
}

// Serialize the node and embed <script> / <payload> children so behavior
// persists through save/load. Called whenever the graph is saved (including
// immediately after palette drop, so new nodes carry their starter script).
xmlNodePtr ScriptedNode::write(xmlDocPtr doc, xmlNodePtr repr) const
{
    xmlNodePtr nodeElement = Node::write(doc, repr);
    if (!nodeElement) {
        return nullptr;
    }

    if (!m_script.isEmpty()) {
        xmlNodePtr scriptNode = xmlNewChild(nodeElement, nullptr, BAD_CAST "script", nullptr);
        xmlSetProp(scriptNode, BAD_CAST "language", BAD_CAST "javascript");
        QByteArray utf8 = m_script.toUtf8();
        xmlNodeSetContent(scriptNode, BAD_CAST utf8.constData());
    }

    if (!m_payload.isEmpty()) {
        QJsonDocument payloadDoc = QJsonDocument::fromVariant(m_payload);
        QByteArray payloadJson = payloadDoc.toJson(QJsonDocument::Compact);
        xmlNodePtr payloadNode = xmlNewChild(nodeElement, nullptr, BAD_CAST "payload", nullptr);
        xmlSetProp(payloadNode, BAD_CAST "format", BAD_CAST "json");
        xmlNodeSetContent(payloadNode, BAD_CAST payloadJson.constData());
    }

    return nodeElement;
}

void ScriptedNode::read(xmlNodePtr node)
{
    Node::read(node);

    for (xmlNodePtr child = node ? node->children : nullptr; child; child = child->next) {
        if (child->type != XML_ELEMENT_NODE) {
            continue;
        }

        if (xmlStrcmp(child->name, BAD_CAST "script") == 0) {
            xmlChar* content = xmlNodeGetContent(child);
            if (content) {
                setScript(QString::fromUtf8(reinterpret_cast<const char*>(content)));
                xmlFree(content);
            }
        } else if (xmlStrcmp(child->name, BAD_CAST "payload") == 0) {
            xmlChar* content = xmlNodeGetContent(child);
            if (content) {
                QByteArray payloadJson(reinterpret_cast<const char*>(content));
                QJsonParseError parseError{};
                QJsonDocument doc = QJsonDocument::fromJson(payloadJson, &parseError);
                if (parseError.error == QJsonParseError::NoError && doc.isObject()) {
                    setPayload(doc.toVariant().toMap());
                }
                xmlFree(content);
            }
        }
    }
}

void ScriptedNode::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Node::paint(painter, option, widget);

    if (m_displayLabel.isEmpty()) {
        return;
    }

    painter->save();
    painter->setPen(QPen(Qt::black));
    static const QFont labelFont("Arial", 7);
    painter->setFont(labelFont);

    QRectF rect = boundingRect().adjusted(4, boundingRect().height() * 0.65, -4, -4);
    painter->drawText(rect, Qt::AlignCenter, m_displayLabel);
    painter->restore();
}

void ScriptedNode::compileIfNeeded()
{
    if (!s_engine) {
        qWarning() << "ScriptedNode: no shared QJSEngine set";
        return;
    }
    if (m_compiledFunction.isCallable() || m_script.trimmed().isEmpty()) {
        return;
    }

    const QString wrapped = QStringLiteral("(function(node, context) {\n%1\n})").arg(m_script);
    m_compiledFunction = s_engine->evaluate(wrapped, QStringLiteral("scripted_node.js"));

    if (m_compiledFunction.isError()) {
        qWarning() << "ScriptedNode: failed to compile script"
                   << m_compiledFunction.property("message").toString();
    }
}

QJSValue ScriptedNode::callScript(const QVariantMap& context)
{
    if (!s_engine) {
        qWarning() << "ScriptedNode: cannot execute script without shared QJSEngine";
        return QJSValue();
    }

    compileIfNeeded();
    if (!m_compiledFunction.isCallable()) {
        return QJSValue();
    }

    ScriptNodeApi api(this);
    QJSValue jsNode = s_engine->newQObject(&api);
    QJSValue jsContext = s_engine->toScriptValue(context);

    QJSValueList args;
    args << jsNode << jsContext;
    return m_compiledFunction.call(args);
}

#include "scripted_node.moc"
