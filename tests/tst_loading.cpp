#include <QtTest>
#include <QFile>
#include <QLoggingCategory>
#include <QTemporaryDir>
#include <QUndoStack>
#include <libxml/tree.h>
#include <memory>

#include "edge.h"
#include "graph.h"
#include "graph_factory.h"
#include "node.h"
#include "scene.h"
#include "socket.h"
#include "window.h"
#include "xml_autosave_observer.h"

namespace {
const char* sourceId = "11111111-1111-4111-8111-111111111111";
const char* sinkId = "22222222-2222-4222-8222-222222222222";

QByteArray graphXml(bool wrapped = false)
{
    const QByteArray nodes =
        "<node id=\"11111111-1111-4111-8111-111111111111\" type=\"SOURCE\" inputs=\"0\" outputs=\"1\" x=\"120\" y=\"80\"/>"
        "<node id=\"22222222-2222-4222-8222-222222222222\" type=\"SINK\" inputs=\"1\" outputs=\"0\"/>";
    const QByteArray edges =
        "<edge id=\"33333333-3333-4333-8333-333333333333\" fromNode=\"11111111-1111-4111-8111-111111111111\" toNode=\"22222222-2222-4222-8222-222222222222\" fromSocketIndex=\"0\" toSocketIndex=\"0\"/>";
    return wrapped ? "<graph><nodes>" + nodes + "</nodes><connections>" + edges + "</connections></graph>"
                   : "<graph>" + nodes + edges + "</graph>";
}

struct World {
    std::unique_ptr<xmlDoc, decltype(&xmlFreeDoc)> doc{xmlNewDoc(BAD_CAST "1.0"), &xmlFreeDoc};
    Scene scene;
    GraphFactory factory{&scene, doc.get()};
    Graph graph{&scene, &factory};
    World()
    {
        xmlDocSetRootElement(doc.get(), xmlNewNode(nullptr, BAD_CAST "graph"));
        scene.setGraphFactory(&factory);
    }
    ~World() { scene.clearGraph(); }
};

struct Observer : GraphObserver {
    int batches = 0;
    void onBatchEnded() override { ++batches; }
};
}

class LoadingTests : public QObject {
    Q_OBJECT
    QTemporaryDir directory;

    QString writeFile(const QString& name, const QByteArray& content)
    {
        const QString path = directory.filePath(name);
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly) || file.write(content) != content.size()) {
            return {};
        }
        return path;
    }

private slots:
    void initTestCase()
    {
        QLoggingCategory::setFilterRules(QStringLiteral("*.debug=false"));
    }

    void rejectedLoadPreservesDocument_data()
    {
        QTest::addColumn<QByteArray>("xml");
        QTest::newRow("malformed") << QByteArray("<graph>");
        QTest::newRow("wrong-root") << QByteArray("<other/>");
        QTest::newRow("dtd") << QByteArray("<!DOCTYPE graph [<!ENTITY name 'value'>]><graph/>");
        auto changed = [](const char* before, const char* after) {
            QByteArray xml = graphXml();
            return xml.replace(before, after);
        };
        QTest::newRow("unknown-type") << changed("SOURCE", "UNKNOWN");
        QTest::newRow("invalid-uuid") << changed(sourceId, "invalid");
        QTest::newRow("duplicate-node") << changed(sinkId, sourceId);
        QTest::newRow("negative-count") << changed("outputs=\"1\"", "outputs=\"-1\"");
        QTest::newRow("nonnumeric-count") << changed("outputs=\"1\"", "outputs=\"abc\"");
        QTest::newRow("excessive-count") << changed("outputs=\"1\"", "outputs=\"2147483647\"");
        QTest::newRow("nonfinite-coordinate") << changed("x=\"120\"", "x=\"nan\"");
        QTest::newRow("missing-reference") << changed("toNode=\"22222222-2222-4222-8222-222222222222\"", "toNode=\"44444444-4444-4444-8444-444444444444\"");
        QTest::newRow("self-loop") << changed("toNode=\"22222222-2222-4222-8222-222222222222\"", "toNode=\"11111111-1111-4111-8111-111111111111\"");
        QTest::newRow("negative-index") << changed("fromSocketIndex=\"0\"", "fromSocketIndex=\"-1\"");
        QTest::newRow("nonnumeric-index") << changed("fromSocketIndex=\"0\"", "fromSocketIndex=\"abc\"");
        QTest::newRow("out-of-range-index") << changed("fromSocketIndex=\"0\"", "fromSocketIndex=\"5\"");
        QTest::newRow("wrong-role") << changed("type=\"SOURCE\" inputs=\"0\" outputs=\"1\"", "type=\"SOURCE\" inputs=\"1\" outputs=\"0\"");
        QByteArray duplicate = graphXml();
        const QByteArray edge = duplicate.mid(duplicate.indexOf("<edge"), duplicate.indexOf("</graph>") - duplicate.indexOf("<edge"));
        duplicate.replace("</graph>", edge + "</graph>");
        QTest::newRow("duplicate-edge-id") << duplicate;
        QByteArray secondEdge = edge;
        secondEdge.replace("33333333-3333-4333-8333-333333333333", "44444444-4444-4444-8444-444444444444");
        QTest::newRow("occupied-socket") << QByteArray(graphXml()).replace("</graph>", secondEdge + "</graph>");
    }

    void rejectedLoadPreservesDocument()
    {
        QFETCH(QByteArray, xml);
        World world;
        QVERIFY(world.graph.loadFromFile(writeFile("seed.xml", graphXml())));
        Node* source = world.scene.getNode(QUuid(sourceId));
        source->setSelected(true);
        world.scene.startGhostEdge(source->getOutputSockets().first(), source->pos());
        const QString before = world.graph.toXml();
        Observer observer;
        world.scene.attach(&observer);
        QSignalSpy changed(&world.scene, &Scene::sceneChanged);
        QSignalSpy loaded(&world.graph, &Graph::graphLoaded);
        const QString path = writeFile("invalid.xml", xml);
        QVERIFY(!world.graph.loadFromFile(path));
        QVERIFY(!world.factory.loadFromXmlFile(path));
        QCOMPARE(world.graph.toXml(), before);
        QCOMPARE(world.scene.getNode(QUuid(sourceId)), source);
        QVERIFY(source->isSelected());
        QVERIFY(world.scene.ghostEdgeActive());
        QCOMPARE(changed.count(), 0);
        QCOMPARE(loaded.count(), 0);
        QCOMPARE(observer.batches, 0);
        QVERIFY(!world.graph.isBatchMode());
    }

    void missingFilePreservesDocument()
    {
        World world;
        const QString id = world.graph.createNode("SOURCE", 10, 20);
        QVERIFY(!world.graph.loadFromFile(directory.filePath("missing.xml")));
        QVERIFY(world.scene.getNode(QUuid(id)));
    }

    void wrappedAndDirectLoadsReplace_data()
    {
        QTest::addColumn<bool>("wrapped");
        QTest::newRow("direct") << false;
        QTest::newRow("autosave-wrappers") << true;
    }

    void wrappedAndDirectLoadsReplace()
    {
        QFETCH(bool, wrapped);
        World world;
        const QString stray = world.graph.createNode("SOURCE", 0, 0);
        Observer observer;
        world.scene.attach(&observer);
        QSignalSpy changed(&world.scene, &Scene::sceneChanged);
        const QString path = writeFile("graph.xml", graphXml(wrapped));
        QVERIFY(world.factory.loadFromXmlFile(path));
        QVERIFY(!world.scene.getNode(QUuid(stray)));
        QCOMPARE(world.scene.getNodes().size(), 2);
        QCOMPARE(world.scene.getEdges().size(), 1);
        Edge* edge = world.scene.getEdges().constBegin().value();
        QVERIFY(edge->getFromSocket() && edge->getToSocket());
        QCOMPARE(edge->getFromSocket()->getConnectedEdge(), edge);
        QCOMPARE(edge->getToSocket()->getConnectedEdge(), edge);
        QCOMPARE(observer.batches, 1);
        QCOMPARE(changed.count(), 1);
        QVERIFY(world.factory.loadFromXmlFile(path));
        QCOMPARE(world.scene.getNodes().size(), 2);
        QCOMPARE(world.scene.getEdges().size(), 1);
    }

    void autosaveRoundTrip()
    {
        World world;
        const QString path = directory.filePath("autosave.xml");
        XmlAutosaveObserver observer(&world.scene, path);
        world.scene.attach(&observer);
        const QString source = world.graph.createNode("SOURCE", 12.5, -7.25);
        const QString sink = world.graph.createNode("SINK", 200, 0);
        const QString edge = world.graph.connectNodes(source, 0, sink, 0);
        QVERIFY(!edge.isEmpty());
        observer.saveNow();
        observer.setEnabled(false);
        world.graph.clearGraph();
        QVERIFY(world.graph.loadFromFile(path));
        QCOMPARE(world.scene.getNodes().size(), 2);
        QCOMPARE(world.scene.getEdges().size(), 1);
        QCOMPARE(world.scene.getNode(QUuid(source))->pos(), QPointF(12.5, -7.25));
        QVERIFY(world.scene.getEdge(QUuid(edge))->getFromSocket());
    }

    void emptyGraphCancelsGhostDrag()
    {
        World world;
        Node* node = world.factory.createNode("SOURCE", QPointF());
        world.scene.startGhostEdge(node->getOutputSockets().first(), node->pos());
        QVERIFY(world.graph.loadFromFile(writeFile("empty.xml", "<graph/>")));
        QVERIFY(world.scene.items().isEmpty());
        QVERIFY(!world.scene.ghostEdgeActive());
    }

    void unicodeFilenameAndMetadata()
    {
        World world;
        QVERIFY(world.graph.loadFromFile(writeFile("seed.xml", graphXml())));
        const QString id = QString::fromLatin1(sourceId);
        const QString script = QStringLiteral("return node.payloadValue('answer') < 50 && context.enabled ? 42 : 0;");
        const QVariantMap payload{{"answer", 41}, {"text", QStringLiteral("A < B & C")}};
        QVERIFY(world.graph.setNodeScript(id, script));
        QVERIFY(world.graph.setNodePayload(id, payload));
        const QString path = writeFile(QString::fromUtf8("graph-\xc3\xa9-\xe6\x97\xa5\xe6\x9c\xac.xml"), world.graph.toXml().toUtf8());
        QVERIFY(world.graph.loadFromFile(path));
        QCOMPARE(world.graph.getNodeScript(id), script);
        QCOMPARE(world.graph.getNodePayload(id), payload);
        QCOMPARE(world.graph.executeNodeScript(id, {{"enabled", true}}).toInt(), 42);
    }

    void windowKeepsUndoOnFailure()
    {
        QTemporaryDir currentDirectory;
        const QString previousDirectory = QDir::currentPath();
        struct RestoreDirectory {
            QString path;
            ~RestoreDirectory() { QDir::setCurrent(path); }
        } restore{previousDirectory};
        QVERIFY(QDir::setCurrent(currentDirectory.path()));
        Window window;
        GraphFactory factory(window.getScene(), nullptr);
        window.adoptFactory(&factory);
        window.createInputNode();
        auto* undo = window.findChild<QUndoStack*>();
        QVERIFY(undo && undo->canUndo());
        window.setCurrentFile("original.xml");
        const QString before = window.getGraph()->toXml();
        QVERIFY(!window.loadGraph(writeFile("bad-window.xml", "<broken/>")));
        QCOMPARE(window.getCurrentFile(), QString("original.xml"));
        QCOMPARE(window.getGraph()->toXml(), before);
        QVERIFY(undo->canUndo());
        QVERIFY(window.loadGraph(writeFile("good-window.xml", graphXml())));
        QVERIFY(!undo->canUndo());
        window.close();
    }
};

QTEST_MAIN(LoadingTests)
#include "tst_loading.moc"
