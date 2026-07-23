#include "scripted_node.h"

#include <atomic>
#include <chrono>
#include <memory>
#include <thread>

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

/**
 * @brief QObject wrapper exposed to JavaScript so scripts interact with C++ safely.
 *
 * Every method here is intentional: keep the surface area tight so scripts
 * can read/write payloads, log, run synthetic workloads, and inspect basic
 * metadata (id/type/sockets/edges) without poking raw C++ pointers.
 */
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

    QString label() const
    {
        if (!m_node) {
            return {};
        }
        return m_node->displayLabel();
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
        map.insert(QStringLiteral("label"), label());
        map.insert(QStringLiteral("inputCount"), m_node->getInputSockets().size());
        map.insert(QStringLiteral("outputCount"), m_node->getOutputSockets().size());
        map.insert(QStringLiteral("edgeCount"), m_node->getIncidentEdges().size());
        return map;
    }

private:
    ScriptedNode* m_node;
};

ScriptEngine ScriptedNode::s_engine;
int ScriptedNode::s_executionDepth = 0;

namespace {
// Max nesting for evaluate(): a script can reenter via graph.executeNodeScript;
// each level is a fresh JS entry so the JS engine's own recursion limit never
// trips - the native stack would overflow first. 8 is generous for legitimate
// node-to-node script calls.
constexpr int kMaxExecutionDepth = 8;
// Watchdog timeout for runaway scripts (applies at the outermost evaluate()).
// Namespace scope so the watchdog lambda needs no capture (MSVC C3493).
constexpr int kWatchdogMs = 5000;
}

ScriptedNode::ScriptedNode(const QUuid& id, const QPointF& position)
    : Node(id, position)
{
    setNodeType(QStringLiteral("SCRIPT"));
}

void ScriptedNode::setSharedEngine(ScriptEngine engine)
{
    s_engine = std::move(engine);
}

void ScriptedNode::setScript(const QString& code)
{
    m_script = code;
    m_compiledFunction = ScriptFunction();
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
 * @param context Key/value map forwarded to the script as its second argument.
 * @return QVariant result provided by the script (cached in m_lastResult).
 */
QVariant ScriptedNode::evaluate(const QVariantMap& context)
{
    // Cap native recursion: a script can reenter evaluate() via
    // graph.executeNodeScript(...); each level is a fresh JS entry so the JS
    // engine's own limit never trips - the native stack would overflow first.
    if (s_executionDepth >= kMaxExecutionDepth) {
        qWarning() << "ScriptedNode: max script recursion depth" << kMaxExecutionDepth
                   << "exceeded - refusing nested evaluate";
        return {};
    }

    compileIfNeeded();
    if (!m_compiledFunction.isValid()) {
        return {};
    }

    const bool isOutermost = (s_executionDepth == 0);

    // RAII depth tracking; Graph refuses destructive ops while this is >0.
    struct DepthGuard {
        int& depth;
        explicit DepthGuard(int& d) : depth(d) { ++depth; }
        ~DepthGuard() { --depth; }
    } guard(s_executionDepth);

    // Watchdog: scripts run synchronously on the UI thread, so a script stuck
    // in an infinite loop would freeze the app forever. Arm a one-shot timer
    // thread on the outermost call (nested calls share it) that interrupts the
    // engine after a timeout. The done-flag and the engine handle are
    // shared_ptr-owned, so the detached thread is safe however we return.
    std::shared_ptr<std::atomic<bool>> scriptDone;
    if (isOutermost && s_engine) {
        scriptDone = std::make_shared<std::atomic<bool>>(false);
        ScriptEngine engineCopy = s_engine; // keeps the backend heap alive
        std::thread([scriptDone, engineCopy]() mutable {
            for (int elapsed = 0; elapsed < kWatchdogMs; elapsed += 100) {
                if (scriptDone->load(std::memory_order_relaxed)) {
                    return;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            engineCopy.interrupt(); // thread-safe per QJSEngine docs
        }).detach();
    }

    ScriptNodeApi api(this);
    // Copy before calling: a script may call graph.setNodeScript() on itself,
    // resetting m_compiledFunction mid-flight; the local keeps the compiled
    // callable alive until this call returns.
    const ScriptFunction fn = m_compiledFunction;
    const ScriptResult result = fn.call(&api, context);

    if (scriptDone) {
        scriptDone->store(true, std::memory_order_relaxed);
    }
    if (!result.ok()) {
        qWarning() << "ScriptedNode: script error in"
                   << getId().toString(QUuid::WithoutBraces)
                   << result.error;
        m_lastResult.clear();
        return {};
    }

    m_lastResult = result.value;
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

/**
 * @brief Serialize the node, embedding <script>/<payload> children.
 *
 * Called whenever the graph is saved (including immediately after palette drop,
 * so starter scripts persist). This is what lets autosave capture script edits.
 */
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
        qWarning() << "ScriptedNode: no shared ScriptEngine set";
        return;
    }
    if (m_compiledFunction.isValid() || m_script.trimmed().isEmpty()) {
        return;
    }

    QString error;
    m_compiledFunction = s_engine.compile(m_script, &error);

    if (!error.isEmpty()) {
        qWarning() << "ScriptedNode: failed to compile script" << error;
    }
}

#include "scripted_node.moc"
