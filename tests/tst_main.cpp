// tests/tst_main.cpp
//
// NodeGraphTests - automatic verification suite for the code-review
// remediation (ADVISORY.md Entries 001/003/004).
//
// Revives the project's abandoned Qt Test skeleton (CMakeLists.txt had it
// commented out) with a console test app covering the fixes that a script
// cannot reach through --script (lifetime, autosave, ghost edge) plus the
// facade/script-safety checks as durable regression tests.
//
// Run:  ctest --test-dir build_Debug -C Debug --output-on-failure
//   or: NodeGraphTests.exe            (exit code = number of failed classes)

#include <QtTest>
#include <QApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QTemporaryDir>

#include "scene.h"
#include "graph.h"
#include "graph_factory.h"
#include "graph_observer.h"
#include "scripted_node.h"
#include "synthetic_work.h"
#include "view.h"
#include "window.h"
#include "xml_autosave_observer.h"
#include "node.h"
#include "edge.h"
#include "socket.h"

#include <QAction>
#include <QGraphicsSceneMouseEvent>
#include <QScrollBar>
#include <QUndoStack>
#include <cmath>
#include <limits>
#include <libxml/tree.h>

// ---------------------------------------------------------------------------
// Fixture: a minimal app world (scene + factory + facade), like main() builds.
// ---------------------------------------------------------------------------
struct TestWorld
{
    Scene* scene = nullptr;
    xmlDocPtr doc = nullptr;
    GraphFactory* factory = nullptr;
    Graph* graph = nullptr;

    TestWorld()
    {
        scene = new Scene; // no parent: owned manually
        doc = xmlNewDoc(BAD_CAST "1.0");
        xmlNodePtr root = xmlNewNode(nullptr, BAD_CAST "graph");
        xmlDocSetRootElement(doc, root);
        xmlSetProp(root, BAD_CAST "version", BAD_CAST "1.0");
        factory = new GraphFactory(scene, doc);
        scene->setGraphFactory(factory);
        graph = new Graph(scene, factory);
    }

    ~TestWorld()
    {
        delete graph;    // resets the nodes' shared engine handle
        delete factory;
        xmlFreeDoc(doc);
        delete scene;    // deletes all remaining items (post-B1: any order is safe)
    }
};

// Pass-through topology (SOURCE -> TRANSFORM -> SINK). Socket indices are the
// GLOBAL per-node indices this app serializes (inputs first, then outputs):
// SOURCE out = 0, TRANSFORM in = 0 / out = 1, SINK in = 0.
static const char* kPassThroughXml = R"XML(<?xml version="1.0" encoding="UTF-8"?>
<graph version="1.0">
  <node id="11111111-1111-4111-8111-111111111111" x="0" y="0" type="SOURCE" inputs="0" outputs="1"/>
  <node id="22222222-2222-4222-8222-222222222222" x="250" y="0" type="TRANSFORM" inputs="1" outputs="1"/>
  <node id="33333333-3333-4333-8333-333333333333" x="500" y="0" type="SINK" inputs="1" outputs="0"/>
  <edge id="44444444-4444-4444-8444-444444444444" fromNode="11111111-1111-4111-8111-111111111111" toNode="22222222-2222-4222-8222-222222222222" fromSocketIndex="0" toSocketIndex="0"/>
  <edge id="55555555-5555-4555-8555-555555555555" fromNode="22222222-2222-4222-8222-222222222222" toNode="33333333-3333-4333-8333-333333333333" fromSocketIndex="1" toSocketIndex="0"/>
</graph>
)XML";

static QString writeTempFile(QTemporaryDir& dir, const QString& name, const QByteArray& content)
{
    const QString path = dir.filePath(name);
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly)) {
        return QString();
    }
    f.write(content);
    return path;
}

// ===========================================================================
// 1. Factory load behavior (A1)
// ===========================================================================
class FactoryLoadTests : public QObject
{
    Q_OBJECT
private slots:
    void init() { m_world = new TestWorld; m_dir = new QTemporaryDir; }
    void cleanup() { delete m_world; delete m_dir; }

    void passThroughChainLoads()
    {
        const QString path = writeTempFile(*m_dir, "passthrough.xml", kPassThroughXml);
        QVERIFY2(!path.isEmpty(), "failed to write fixture file");
        QVERIFY2(m_world->factory->loadFromXmlFile(path),
                 "pass-through topology must load (ADVISORY A1)");
        QCOMPARE(m_world->scene->getNodes().size(), 3);
        QCOMPARE(m_world->scene->getEdges().size(), 2);
    }

    void trueDuplicateStillRejected()
    {
        // Two edges out of the SAME output socket is a real duplicate and must
        // still be rejected after the direction-prefix fix (A1 must not weaken
        // true-positive detection).
        const char* dupXml = R"XML(<?xml version="1.0" encoding="UTF-8"?>
<graph version="1.0">
  <node id="11111111-1111-4111-8111-111111111111" x="0" y="0" type="SOURCE" inputs="0" outputs="1"/>
  <node id="22222222-2222-4222-8222-222222222222" x="250" y="0" type="SINK" inputs="1" outputs="0"/>
  <node id="33333333-3333-4333-8333-333333333333" x="500" y="0" type="SINK" inputs="1" outputs="0"/>
  <edge id="44444444-4444-4444-8444-444444444444" fromNode="11111111-1111-4111-8111-111111111111" toNode="22222222-2222-4222-8222-222222222222" fromSocketIndex="0" toSocketIndex="0"/>
  <edge id="55555555-5555-4555-8555-555555555555" fromNode="11111111-1111-4111-8111-111111111111" toNode="33333333-3333-4333-8333-333333333333" fromSocketIndex="0" toSocketIndex="0"/>
</graph>
)XML";
        const QString path = writeTempFile(*m_dir, "dup.xml", dupXml);
        QVERIFY(!path.isEmpty());
        QVERIFY2(!m_world->factory->loadFromXmlFile(path),
                 "two edges on one output socket must be rejected");
        QCOMPARE(m_world->scene->getNodes().size(), 0); // all-or-nothing
    }

private:
    TestWorld* m_world = nullptr;
    QTemporaryDir* m_dir = nullptr;
};

// ===========================================================================
// 2. Facade behavior (B2, C5, C6, C7)
// ===========================================================================
class FacadeTests : public QObject
{
    Q_OBJECT
private slots:
    void init() { m_world = new TestWorld; m_dir = new QTemporaryDir; }
    void cleanup() { delete m_world; delete m_dir; }

    void duplicateConnectRefused() // B2
    {
        const QString src = m_world->graph->createNode("SOURCE", 0, 0);
        const QString dst = m_world->graph->createNode("SINK", 200, 0);
        const QString dst2 = m_world->graph->createNode("SINK", 400, 0);
        QVERIFY(!src.isEmpty() && !dst.isEmpty() && !dst2.isEmpty());
        QVERIFY(!m_world->graph->connectNodes(src, 0, dst, 0).isEmpty());
        const QString dup = m_world->graph->connectNodes(src, 0, dst2, 0);
        QVERIFY2(dup.isEmpty(), "second connect on an occupied output socket must fail");
        QCOMPARE(m_world->scene->getEdges().size(), 1);
    }

    void connectNodesUsesGlobalSocketIndices()
    {
        // TRANSFORM sockets are global 0 (input) and 1 (output): the scripting
        // API, getEdgeData(), XML, and undo all speak the same global scheme.
        const QString t1 = m_world->graph->createNode("TRANSFORM", 0, 0);
        const QString t2 = m_world->graph->createNode("TRANSFORM", 300, 0);
        QVERIFY(!t1.isEmpty() && !t2.isEmpty());

        QVERIFY2(m_world->graph->connectNodes(t1, 0, t2, 0).isEmpty(),
                 "global index 0 on TRANSFORM is its input, not an output");

        const QString edge = m_world->graph->connectNodes(t1, 1, t2, 0);
        QVERIFY2(!edge.isEmpty(), "TRANSFORM output is global index 1");

        const QVariantMap data = m_world->graph->getEdgeData(edge);
        QCOMPARE(data["fromSocketIndex"].toInt(), 1);
        QCOMPARE(data["toSocketIndex"].toInt(), 0);
    }

    void multiSocketGlobalIndices()
    {
        // SPLIT: in 0, out 1, out 2. MERGE: in 0/1, out 2.
        const QString split = m_world->graph->createNode("SPLIT", 0, 0);
        const QString t1 = m_world->graph->createNode("TRANSFORM", 300, -100);
        const QString t2 = m_world->graph->createNode("TRANSFORM", 300, 100);
        QVERIFY(!split.isEmpty() && !t1.isEmpty() && !t2.isEmpty());
        QVERIFY(!m_world->graph->connectNodes(split, 1, t1, 0).isEmpty());
        QVERIFY(!m_world->graph->connectNodes(split, 2, t2, 0).isEmpty());
        QVERIFY2(m_world->graph->connectNodes(split, 0, t1, 0).isEmpty(),
                 "SPLIT global index 0 is its input");

        const QString merge = m_world->graph->createNode("MERGE", 600, 0);
        const QString sink = m_world->graph->createNode("SINK", 900, 0);
        QVERIFY(!merge.isEmpty() && !sink.isEmpty());
        QVERIFY(!m_world->graph->connectNodes(t1, 1, merge, 0).isEmpty());
        QVERIFY(!m_world->graph->connectNodes(t2, 1, merge, 1).isEmpty());
        QVERIFY(!m_world->graph->connectNodes(merge, 2, sink, 0).isEmpty());
        QCOMPARE(m_world->scene->getEdges().size(), 5);
    }

    void dropNodeAnimatesIntoPlace()
    {
        const QString id = m_world->graph->dropNode("SINK", 320, 240, 120);
        QVERIFY(!id.isEmpty());
        Node* node = m_world->scene->getNode(QUuid(id));
        QVERIFY(node);

        // Starts offset and faded, like something just thrown onto the canvas.
        QVERIFY(node->opacity() < 0.5);
        QVERIFY((node->pos() - QPointF(320, 240)).manhattanLength() > 20.0);

        QTRY_VERIFY_WITH_TIMEOUT(
            (node->pos() - QPointF(320, 240)).manhattanLength() < 0.5, 2000);
        QVERIFY2(node->opacity() > 0.99, "drop must end fully opaque");
    }

    void dropNodeRejectsInvalidType()
    {
        QSignalSpy errors(m_world->graph, &Graph::errorOccurred);
        QVERIFY(m_world->graph->dropNode("NOPE", 0, 0, 100).isEmpty());
        QVERIFY(errors.count() >= 1);
    }

    void dropNodeDeletedMidAnimationIsSafe()
    {
        const QString id = m_world->graph->dropNode("SINK", 100, 100, 400);
        QVERIFY(!id.isEmpty());
        QVERIFY(m_world->graph->deleteNode(id));
        QTest::qWait(700); // animation ticks must not touch the freed node
        QVERIFY(true);
    }

    void nonFiniteCoordinatesAreRejected()
    {
        // Non-finite positions poisoned the scene and serialized as nan/inf,
        // which the loader then refused - save/load round-trip was broken.
        const qreal nan = std::numeric_limits<qreal>::quiet_NaN();
        const qreal inf = std::numeric_limits<qreal>::infinity();

        QSignalSpy errors(m_world->graph, &Graph::errorOccurred);
        QVERIFY(m_world->graph->createNode("SOURCE", nan, 0).isEmpty());
        QVERIFY(m_world->graph->createNode("SOURCE", inf, 0).isEmpty());
        QVERIFY(m_world->graph->createNode("SOURCE", 0, -inf).isEmpty());
        QCOMPARE(m_world->scene->getNodes().size(), 0);

        const QString id = m_world->graph->createNode("SOURCE", 10, 10);
        QVERIFY(!id.isEmpty());
        QVERIFY(!m_world->graph->moveNode(id, nan, 0));
        QVERIFY(!m_world->graph->moveNode(id, inf, inf)); // finite delta, inf result
        QVERIFY(!m_world->graph->setNodePosition(id, inf, inf));
        QCOMPARE(m_world->scene->getNode(QUuid(id))->pos(), QPointF(10, 10));

        // What survives must round-trip through save/load unchanged.
        const QString path = writeTempFile(*m_dir, "finite.xml",
                                           m_world->graph->toXml().toUtf8());
        QVERIFY(!path.isEmpty());
        m_world->graph->clearGraph();
        QVERIFY(m_world->graph->loadFromFile(path));
        QCOMPARE(m_world->scene->getNodes().size(), 1);
        QVERIFY(errors.count() >= 6);
    }

    void facadeMutationsAreUndoable()
    {
        QUndoStack stack;
        m_world->graph->setUndoStack(&stack);

        const QString a = m_world->graph->createNode("SOURCE", 0, 0);
        const QString b = m_world->graph->createNode("TRANSFORM", 200, 0);
        const QString c = m_world->graph->createNode("SINK", 400, 0);
        const QString e1 = m_world->graph->connectNodes(a, 0, b, 0);
        const QString e2 = m_world->graph->connectNodes(b, 1, c, 0);
        QVERIFY(!a.isEmpty() && !b.isEmpty() && !c.isEmpty());
        QVERIFY(!e1.isEmpty() && !e2.isEmpty());
        QCOMPARE(stack.count(), 5);
        QVERIFY(m_world->graph->canUndo());

        // Connections: undo/redo preserves the original edge UUIDs
        QVERIFY(m_world->graph->undo());
        QCOMPARE(m_world->scene->getEdges().size(), 1);
        QVERIFY(m_world->graph->undo());
        QCOMPARE(m_world->scene->getEdges().size(), 0);
        QVERIFY(m_world->graph->redo());
        QVERIFY(m_world->graph->redo());
        QCOMPARE(m_world->scene->getEdges().size(), 2);
        QVERIFY(m_world->scene->getEdge(QUuid(e1)) != nullptr);
        QVERIFY(m_world->scene->getEdge(QUuid(e2)) != nullptr);

        // Deleting a middle node takes its edges; undo restores all three
        QVERIFY(m_world->graph->deleteNode(b));
        QCOMPARE(m_world->scene->getNodes().size(), 2);
        QCOMPARE(m_world->scene->getEdges().size(), 0);
        QVERIFY(m_world->graph->undo());
        QCOMPARE(m_world->scene->getNodes().size(), 3);
        QCOMPARE(m_world->scene->getEdges().size(), 2);
        QVERIFY(m_world->scene->getNode(QUuid(b)) != nullptr);
        QVERIFY(m_world->scene->getEdge(QUuid(e1)) != nullptr);
        QVERIFY(m_world->graph->redo());
        QCOMPARE(m_world->scene->getNodes().size(), 2);

        // A batch is a single undo step
        m_world->graph->beginBatch();
        const QString d1 = m_world->graph->createNode("TRANSFORM", 600, 0);
        const QString d2 = m_world->graph->createNode("TRANSFORM", 800, 0);
        m_world->graph->endBatch();
        QCOMPARE(m_world->scene->getNodes().size(), 4);
        QVERIFY(m_world->graph->undo()); // one step removes both
        QCOMPARE(m_world->scene->getNodes().size(), 2);
        QVERIFY(m_world->graph->redo());
        QCOMPARE(m_world->scene->getNodes().size(), 4);
        QVERIFY(m_world->scene->getNode(QUuid(d1)) != nullptr);
        QVERIFY(m_world->scene->getNode(QUuid(d2)) != nullptr);

        // Moves are undoable
        Node* moved = m_world->scene->getNode(QUuid(d1));
        const QPointF before = moved->pos();
        QVERIFY(m_world->graph->moveNode(d1, 25, 25));
        QCOMPARE(m_world->scene->getNode(QUuid(d1))->pos(), before + QPointF(25, 25));
        QVERIFY(m_world->graph->undo());
        QCOMPARE(m_world->scene->getNode(QUuid(d1))->pos(), before);
        QVERIFY(m_world->graph->redo());
        QCOMPARE(m_world->scene->getNode(QUuid(d1))->pos(), before + QPointF(25, 25));

        // An empty batch leaves no history entry
        const int beforeCount = stack.count();
        m_world->graph->beginBatch();
        m_world->graph->endBatch();
        QCOMPARE(stack.count(), beforeCount);

        m_world->graph->setUndoStack(nullptr); // stack is about to go out of scope
    }

    void snapToGridFacade()
    {
        QVERIFY(!m_world->graph->isSnapToGrid());
        m_world->graph->setSnapToGrid(true);
        QVERIFY(m_world->graph->isSnapToGrid());
        QCOMPARE(m_world->graph->gridSize(), 40);

        const QVariantMap sp = m_world->graph->snapPoint(43.0, -57.0);
        QCOMPARE(sp.value("x").toDouble(), 40.0);
        QCOMPARE(sp.value("y").toDouble(), -40.0);
        const qreal nan = std::numeric_limits<qreal>::quiet_NaN();
        QVERIFY(m_world->graph->snapPoint(nan, 0.0).isEmpty()); // NaN refused

        // Programmatic creation keeps exact coordinates; snapping is explicit
        const QString id = m_world->graph->createNode("SOURCE", 43, -57);
        QVERIFY(!id.isEmpty());
        Node* node = m_world->scene->getNode(QUuid(id));
        QCOMPARE(node->pos(), QPointF(43, -57));
        QVERIFY(m_world->graph->snapNode(id));
        QCOMPARE(node->pos(), QPointF(40, -40));
        QVERIFY(m_world->graph->snapNode(id)); // already on grid: no-op success
        QVERIFY(!m_world->graph->snapNode("bad"));

        // snapNodes() moves every off-grid node in one undoable command
        m_world->graph->createNode("SINK", 201, 37);
        m_world->graph->createNode("SINK", 99, 81);
        QCOMPARE(m_world->graph->snapNodes(), 2);
        for (Node* n : m_world->scene->getNodes().values()) {
            QVERIFY(qFuzzyIsNull(std::fmod(n->pos().x(), 40.0)));
            QVERIFY(qFuzzyIsNull(std::fmod(n->pos().y(), 40.0)));
        }

        // Snapping is undoable when a stack is attached
        QUndoStack stack;
        m_world->graph->setUndoStack(&stack);
        QVERIFY(m_world->graph->setNodePosition(id, 43, -57));
        QVERIFY(m_world->graph->snapNode(id));
        QCOMPARE(m_world->scene->getNode(QUuid(id))->pos(), QPointF(40, -40));
        QVERIFY(m_world->graph->undo());
        QCOMPARE(m_world->scene->getNode(QUuid(id))->pos(), QPointF(43, -57));
        m_world->graph->setUndoStack(nullptr);
    }

    void alignGraphLayersByTopology()
    {
        const QString src = m_world->graph->createNode("SOURCE", 300, 300);
        const QString tr = m_world->graph->createNode("TRANSFORM", 10, 10);
        const QString snk = m_world->graph->createNode("SINK", 700, 100);
        QVERIFY(!src.isEmpty() && !tr.isEmpty() && !snk.isEmpty());
        QVERIFY(!m_world->graph->connectNodes(src, 0, tr, 0).isEmpty());
        QVERIFY(!m_world->graph->connectNodes(tr, 1, snk, 0).isEmpty());

        QUndoStack stack;
        m_world->graph->setUndoStack(&stack);
        QCOMPARE(m_world->graph->alignGraph(), 3);

        Node* s = m_world->scene->getNode(QUuid(src));
        Node* t = m_world->scene->getNode(QUuid(tr));
        Node* k = m_world->scene->getNode(QUuid(snk));
        QVERIFY(s->pos().x() < t->pos().x());
        QVERIFY(t->pos().x() < k->pos().x());
        for (Node* n : {s, t, k}) {
            QVERIFY(qFuzzyIsNull(std::fmod(n->pos().x(), 40.0)));
            QVERIFY(qFuzzyIsNull(std::fmod(n->pos().y(), 40.0)));
        }

        // One undo step restores every original position
        QVERIFY(m_world->graph->undo());
        QCOMPARE(s->pos(), QPointF(300, 300));
        QCOMPARE(t->pos(), QPointF(10, 10));
        QCOMPARE(k->pos(), QPointF(700, 100));
        QVERIFY(m_world->graph->redo());
        QCOMPARE(m_world->graph->alignGraph(), 0); // already aligned
        m_world->graph->setUndoStack(nullptr);
    }

    void sceneRectTracksContent()
    {
        QCOMPARE(m_world->scene->sceneRect(), QRectF(-1000, -1000, 2000, 2000));

        const QString far = m_world->graph->createNode("SOURCE", 5000, 5000);
        QVERIFY(!far.isEmpty());
        QVERIFY2(m_world->scene->sceneRect().contains(QPointF(5000, 5000)),
                 "content far outside the base canvas must expand the scene rect");

        QVERIFY(m_world->graph->setNodePosition(far, -6000, 6000));
        QVERIFY(m_world->scene->sceneRect().contains(QPointF(-6000, 6000)));

        m_world->graph->clearGraph();
        QCOMPARE(m_world->scene->sceneRect(), QRectF(-1000, -1000, 2000, 2000));
    }

    void socketRightPressStaysAccepted()
    {
        // The ghost-edge drag starts with a right press on an output socket.
        // The event must stay accepted so QGraphicsView does not start a
        // rubber band around the ghost edge (the stray rectangle).
        const QString id = m_world->graph->createNode("SOURCE", 0, 0);
        QVERIFY(!id.isEmpty());
        Node* node = m_world->scene->getNode(QUuid(id));
        QVERIFY(node && !node->getOutputSockets().isEmpty());
        Socket* output = node->getOutputSockets().first();

        QGraphicsSceneMouseEvent press(QEvent::GraphicsSceneMousePress);
        press.setButton(Qt::RightButton);
        press.setButtons(Qt::RightButton);
        press.setScenePos(output->scenePos());
        output->mousePressEvent(&press);

        QVERIFY2(press.isAccepted(), "right press must stay accepted");
        QVERIFY2(m_world->scene->ghostEdgeActive(), "ghost edge must start");
        m_world->scene->cancelGhostEdge();
        QVERIFY(!m_world->scene->ghostEdgeActive());
    }

    void facadeLoadReplaces() // C6
    {
        m_world->graph->createNode("SINK", 900, 900); // stray, never saved
        QCOMPARE(m_world->scene->getNodes().size(), 1);
        const QString path = writeTempFile(*m_dir, "pt.xml", kPassThroughXml);
        QVERIFY(!path.isEmpty());
        QVERIFY(m_world->graph->loadFromFile(path));
        QCOMPARE(m_world->scene->getNodes().size(), 3); // replaced, not merged
        QCOMPARE(m_world->scene->getEdges().size(), 2);
    }

    void toXmlIsReal() // C7
    {
        m_world->graph->createNode("SOURCE", 0, 0);
        const QString xml = m_world->graph->toXml();
        QVERIFY(xml.contains("<graph"));
        QVERIFY(xml.contains("SOURCE"));
        QVERIFY(xml != "<graph></graph>");
    }

    void bulkMutationsEmitSceneChangedOnce()
    {
        const QString src = m_world->graph->createNode("SOURCE", 0, 0);
        const QString mid = m_world->graph->createNode("TRANSFORM", 250, 0);
        const QString snk = m_world->graph->createNode("SINK", 500, 0);
        QVERIFY(!src.isEmpty() && !mid.isEmpty() && !snk.isEmpty());
        QVERIFY(!m_world->graph->connectNodes(src, 0, mid, 0).isEmpty());
        QVERIFY(!m_world->graph->connectNodes(mid, 1, snk, 0).isEmpty());

        // One node with two incident edges used to refresh the UI 3 times.
        QSignalSpy changed(m_world->scene, &Scene::sceneChanged);
        QVERIFY(m_world->graph->deleteNode(mid));
        QCOMPARE(changed.count(), 1);

        // A GraphSubject batch defers everything to a single flush.
        changed.clear();
        m_world->graph->beginBatch();
        m_world->graph->deleteNode(src);
        m_world->graph->deleteNode(snk);
        m_world->graph->createNode("SINK", 100, 100);
        QCOMPARE(changed.count(), 0);
        m_world->graph->endBatch();
        QCOMPARE(changed.count(), 1);
    }

    void saveToUnicodePathRoundTrips()
    {
        // libxml's file API takes a narrow path; saveToFile must go through
        // QFile so non-ASCII filenames work on Windows.
        const QString src = m_world->graph->createNode("SOURCE", 0, 0);
        const QString tr = m_world->graph->createNode("TRANSFORM", 250, 0);
        QVERIFY(!src.isEmpty() && !tr.isEmpty());
        QVERIFY(!m_world->graph->connectNodes(src, 0, tr, 0).isEmpty());

        const QString path = m_dir->filePath(
            QString::fromUtf8("sauvegarde-\xc3\xa9-\xe6\x97\xa5\xe6\x9c\xac.xml"));
        QVERIFY2(m_world->graph->saveToFile(path), "save to a Unicode path must succeed");
        QVERIFY(QFile::exists(path));

        QFile saved(path);
        QVERIFY(saved.open(QIODevice::ReadOnly));
        const QByteArray content = saved.readAll();
        QVERIFY2(content.contains("encoding=\"UTF-8\""), "saved XML must declare UTF-8");

        m_world->graph->clearGraph();
        QVERIFY(m_world->graph->loadFromFile(path));
        QCOMPARE(m_world->scene->getNodes().size(), 2);
        QCOMPARE(m_world->scene->getEdges().size(), 1);
    }

    void batchIsRealNow() // C5
    {
        QVERIFY(!m_world->graph->isBatchMode());
        m_world->graph->beginBatch();
        QVERIFY(m_world->graph->isBatchMode());
        m_world->graph->endBatch();
        QVERIFY(!m_world->graph->isBatchMode());
    }

private:
    TestWorld* m_world = nullptr;
    QTemporaryDir* m_dir = nullptr;
};

// ===========================================================================
// 3. Script-host safety (B3, C1, C2)
// ===========================================================================
class ScriptSafetyTests : public QObject
{
    Q_OBJECT
private slots:
    void init() { m_world = new TestWorld; }
    void cleanup() { delete m_world; }

    void scriptCannotDeleteItsOwnNode() // B3
    {
        const QString id = m_world->graph->createNode("TRANSFORM", 0, 0);
        QVERIFY(!id.isEmpty());
        QVERIFY(m_world->graph->setNodeScript(id, "graph.deleteNode(node.nodeId()); 'done'"));
        m_world->graph->executeNodeScript(id, {});
        QVERIFY2(m_world->scene->getNode(QUuid(id)) != nullptr,
                 "deleteNode during script execution must be refused");
    }

    void recursionIsCapped() // C1
    {
        const QString id = m_world->graph->createNode("TRANSFORM", 0, 0);
        QVERIFY(!id.isEmpty());
        QVERIFY(m_world->graph->setNodeScript(id, "graph.executeNodeScript(node.nodeId(), {}); 'done'"));
        m_world->graph->executeNodeScript(id, {}); // must return, not overflow the stack
        QVERIFY(m_world->scene->getNode(QUuid(id)) != nullptr);
    }

    void scriptErrorsAreReported()
    {
        const QString id = m_world->graph->createNode("TRANSFORM", 0, 0);
        QVERIFY(!id.isEmpty());

        // Runtime error: queryable through the facade and emitted as a signal
        QVERIFY(m_world->graph->setNodeScript(id, "throw new Error('boom');"));
        QSignalSpy errors(m_world->graph, &Graph::errorOccurred);
        m_world->graph->executeNodeScript(id, {});
        QVERIFY2(!m_world->graph->getNodeScriptError(id).isEmpty(),
                 "runtime error must be queryable via getNodeScriptError");
        QVERIFY2(errors.count() >= 1, "runtime error must emit errorOccurred");

        // A failed run must not be mistaken for a script returning null
        QVERIFY(m_world->graph->setNodeScript(id, "1 + 1;"));
        m_world->graph->executeNodeScript(id, {});
        QVERIFY2(m_world->graph->getNodeScriptError(id).isEmpty(),
                 "successful run must clear the previous error");

        // Compile failure is reported as well
        QVERIFY(m_world->graph->setNodeScript(id, "function ( {"));
        m_world->graph->executeNodeScript(id, {});
        QVERIFY2(!m_world->graph->getNodeScriptError(id).isEmpty(),
                 "compile failure must be reported");
    }

    void watchdogInterruptsInfiniteScript() // C2 (slow: ~5s by design)
    {
        const QString id = m_world->graph->createNode("TRANSFORM", 0, 0);
        QVERIFY(!id.isEmpty());
        QVERIFY(m_world->graph->setNodeScript(id, "while (1) {}"));
        QElapsedTimer timer;
        timer.start();
        m_world->graph->executeNodeScript(id, {});
        const qint64 elapsed = timer.elapsed();
        QVERIFY2(elapsed >= 4000 && elapsed < 20000,
                 qPrintable(QString("watchdog should interrupt at ~5s, took %1ms").arg(elapsed)));
    }

private:
    TestWorld* m_world = nullptr;
};

// ===========================================================================
// 4. SyntheticWork (A5, C3)
// ===========================================================================
class SyntheticWorkTests : public QObject
{
    Q_OBJECT
private slots:
    void durationIsMeasured() // A5
    {
        const QVariantMap r = SyntheticWork::run({{"task", "delay"}, {"delayMs", 20}});
        QVERIFY2(r.value("durationMs").toLongLong() > 0,
                 "durationMs must be measured after the work, not before");
    }

    void delayIsClamped() // C3 (slow: 5s by design)
    {
        const QVariantMap r = SyntheticWork::run({{"task", "delay"}, {"delayMs", 999999}});
        QCOMPARE(r.value("delayMs").toInt(), 5000);
    }
};

// ===========================================================================
// 5. Observer & batch machinery (B5, C5)
// ===========================================================================
class CountingObserver : public GraphObserver
{
public:
    int added = 0;
    int batchEnded = 0;
    void onNodeAdded(const Node&) override { ++added; }
    void onBatchEnded() override { ++batchEnded; }
};

class ObserverTests : public QObject
{
    Q_OBJECT
private slots:
    void init() { m_world = new TestWorld; }
    void cleanup() { delete m_world; }

    void batchMutesThenFlushes() // C5
    {
        CountingObserver obs;
        m_world->scene->attach(&obs);
        {
            GraphSubject::BatchGuard guard;
            m_world->graph->createNode("SOURCE", 0, 0);
            QCOMPARE(obs.added, 0); // muted during batch
        } // guard ends batch here
        QCOMPARE(obs.batchEnded, 1); // exactly one flush
        m_world->scene->detach(&obs);
    }

    void observerSelfDetachesOnDestroy() // B5
    {
        {
            auto* obs = new CountingObserver;
            m_world->scene->attach(obs);
            delete obs; // no explicit detach: ~GraphObserver must self-detach
        }
        // Would crash pre-fix (dangling observer in the notify loop):
        m_world->graph->createNode("SOURCE", 0, 0);
        QCOMPARE(m_world->scene->getNodes().size(), 1);
    }

private:
    TestWorld* m_world = nullptr;
};

// ===========================================================================
// 6. Lifetime (A2, A3, B1) - the tests --script could not do
// ===========================================================================
class LifetimeTests : public QObject
{
    Q_OBJECT
private slots:
    void init() { m_world = new TestWorld; m_dir = new QTemporaryDir; }
    void cleanup() { delete m_world; delete m_dir; }

    void clearDuringGhostDragIsSafe() // A3
    {
        Node* a = m_world->factory->createNode("SOURCE", QPointF(0, 0));
        Node* b = m_world->factory->createNode("SINK", QPointF(300, 0));
        QVERIFY(a && b);
        QVERIFY(a->outputSockets().size() > 0);
        Socket* out = a->outputSockets().first();
        m_world->scene->startGhostEdge(out, out->mapToScene(out->boundingRect().center()));
        QVERIFY(m_world->scene->ghostEdgeActive());
        m_world->scene->clearGraph(); // deletes the ghost edge AND its source socket
        QVERIFY2(!m_world->scene->ghostEdgeActive(), "clear must cancel the ghost drag");
        m_world->scene->cancelGhostEdge(); // must be a safe no-op, not a double-delete
        QCOMPARE(m_world->scene->getNodes().size(), 0);
    }

    void autosaveSurvivesShutdownClear() // A2
    {
        const QString path = writeTempFile(*m_dir, "autosave_test.xml", "");
        QVERIFY(!path.isEmpty());
        auto* observer = new XmlAutosaveObserver(m_world->scene, path);
        observer->setDelay(300);
        m_world->scene->attach(observer);

        m_world->graph->createNode("SOURCE", 0, 0);
        QTRY_VERIFY_WITH_TIMEOUT(QFile::exists(path) && QFileInfo(path).size() > 100, 5000);

        // What closeEvent does: shutdown clear (fix: guarded so it can't wipe)
        m_world->scene->prepareForShutdown();
        delete observer; // dtor must NOT flush the cleared (empty) scene

        QFile f(path);
        QVERIFY(f.open(QIODevice::ReadOnly));
        const QByteArray content = f.readAll();
        QVERIFY2(content.contains("SOURCE"),
                 "autosave file must survive the shutdown clear (ADVISORY A2)");
    }

    void sceneTeardownWithLiveGraphIsSafe() // B1/B4 shape
    {
        const QString source = m_world->graph->createNode("SOURCE", 0, 0);
        const QString t = m_world->graph->createNode("TRANSFORM", 250, 0);
        const QString s = m_world->graph->createNode("SINK", 500, 0);
        m_world->graph->connectNodes(source, 0, t, 0);
        m_world->graph->connectNodes(t, 1, s, 0);
        QCOMPARE(m_world->scene->getEdges().size(), 2);
        // Deleting the scene outright (base ~QGraphicsScene deletes items in
        // arbitrary order) must not crash: the whole point of B1.
        delete m_world->scene;
        m_world->scene = nullptr; // keep TestWorld dtor from double-deleting
        QVERIFY(true); // survived
    }

private:
    TestWorld* m_world = nullptr;
    QTemporaryDir* m_dir = nullptr;
};

// ===========================================================================
// 7. Window guards (status bar before factory, shortcut ambiguity)
// ===========================================================================
class WindowGuardTests : public QObject
{
    Q_OBJECT
private slots:
    void statusBarBeforeFactoryAdoptionIsSafe()
    {
        // Window constructs with m_graph == nullptr; main() adopts the factory
        // afterwards. updateStatusBar() must not dereference a null facade.
        Window window;
        window.updateStatusBar();
        QVERIFY(true);
    }

    void viewCenterFollowsContent()
    {
        Window window;
        QVERIFY2(!window.getView()->centerOnGraph(), "empty scene has nothing to center on");
        QVERIFY(!window.getView()->centerOnSelection());

        GraphFactory factory(window.getScene(), nullptr);
        window.adoptFactory(&factory);
        const QString node = window.getGraph()->createNode("SOURCE", 500, 500);
        QVERIFY(!node.isEmpty());
        QVERIFY2(window.getView()->centerOnGraph(), "non-empty scene must center");
        QVERIFY2(!window.getView()->centerOnSelection(), "nothing is selected");
    }

    void viewPanByScrolls()
    {
        Window window;
        GraphFactory factory(window.getScene(), nullptr);
        window.adoptFactory(&factory);
        QVERIFY(!window.getGraph()->createNode("SOURCE", 3000, 3000).isEmpty());

        View* view = window.getView();
        view->resize(300, 200);
        QScrollBar* hbar = view->horizontalScrollBar();
        QVERIFY2(hbar->maximum() > 100, "distant content must give the scrollbar range");
        const int before = hbar->value();

        view->panBy(QPoint(-50, 0)); // dragging left scrolls right
        QCOMPARE(hbar->value(), before + 50);
        view->panBy(QPoint(30, 0));
        QCOMPARE(hbar->value(), before + 20);
    }

    void qtActionShortcutsAreNotAmbiguous()
    {
        // Two enabled QActions sharing a key sequence make Qt fire neither and
        // beep ("Ambiguous shortcut overload"). Ctrl+1 used to collide between
        // Add Input and Zoom Reset.
        Window window;
        QHash<QString, QAction*> owner;
        const QList<QAction*> actions = window.findChildren<QAction*>();
        QVERIFY(!actions.isEmpty());
        for (QAction* action : actions) {
            for (const QKeySequence& sequence : action->shortcuts()) {
                const QString key = sequence.toString();
                if (key.isEmpty()) {
                    continue;
                }
                QVERIFY2(!owner.contains(key) || owner.value(key) == action,
                         qPrintable(QString("ambiguous QAction shortcut: %1").arg(key)));
                owner.insert(key, action);
            }
        }
    }
};

// ===========================================================================
// main: run every class, exit code = number of failed classes
// ===========================================================================
int main(int argc, char** argv)
{
    QApplication app(argc, argv);

    // The codebase logs heavily at debug level; keep test output readable.
    qInstallMessageHandler([](QtMsgType type, const QMessageLogContext&, const QString& msg) {
        if (type == QtDebugMsg) {
            return;
        }
        fprintf(stderr, "%s\n", qPrintable(msg));
    });

    int failedClasses = 0;
    const auto run = [&](QObject* tc) {
        const char* name = tc->metaObject()->className();
        qInfo().noquote() << "=== class start:" << name;
        const int failed = QTest::qExec(tc, argc, argv);
        qInfo().noquote() << "=== class done :" << name << "failures:" << failed;
        if (failed != 0) {
            qCritical() << "CLASS FAILED:" << name << "failures:" << failed;
        }
        failedClasses += failed;
        delete tc;
    };

    qInfo() << "=== NodeGraphTests: automatic verification suite ===";
    run(new FactoryLoadTests);
    run(new FacadeTests);
    run(new ScriptSafetyTests);
    run(new SyntheticWorkTests);
    run(new ObserverTests);
    run(new LifetimeTests);
    run(new WindowGuardTests);
    qInfo() << "=== NodeGraphTests done, failed classes:" << failedClasses << "===";

    return failedClasses;
}

#include "tst_main.moc"
