# File-Based Logging System (From Commit History)

## Overview

The old logging system redirected all `qDebug()` output to timestamped log files, making it easy to:
- **Debug application behavior**
- **Trace execution paths**
- **Verify test coverage** (which code was hit)
- **Analyze user interactions**
- **Correlate events** across sessions

## How It Worked

### 1. Log File Creation
- **Directory:** `logs/`
- **Filename Pattern:** `NodeGraph_YYYY-MM-DD_hh-mm-ss.log`
- **Example:** `NodeGraph_2025-10-09_14-23-45.log`

### 2. Message Handler
- Used `qInstallMessageHandler()` to intercept ALL Qt debug messages
- Added timestamps with milliseconds: `[2025-10-09 14:23:45.123]`
- Added severity levels: `DEBUG`, `INFO`, `WARN`, `ERROR`, `FATAL`
- Flushed immediately to ensure no lost messages

### 3. Session Tracking
- Generated unique session ID (UUID first 8 chars)
- Logged session start/end
- Included graph state (node count, edge count)

## Benefits for Test Coverage

### 1. Execution Tracing
```log
[2025-10-09 14:23:45.001] DEBUG: Socket right-clicked: index: 0 role: Output
[2025-10-09 14:23:45.010] DEBUG: GHOST: Started from socket 0 ( Output )
[2025-10-09 14:23:45.234] DEBUG: GHOST: ✓ Created edge 0 → 0
```

### 2. Code Path Verification
- Compare log output to source code
- See which functions were called
- Identify which branches were taken
- Find uncovered code (no log statements = not executed)

### 3. Test Debugging
- When test fails, check log file for exact sequence
- See where code diverged from expected
- Verify state changes

## Implementation (From commits.tar.gz)

### setupLogging() Function

```cpp
void setupLogging()
{
    // Create logs directory if it doesn't exist
    QDir logsDir("logs");
    if (!logsDir.exists()) {
        logsDir.mkpath(".");
    }

    // Create timestamped log file
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd_hh-mm-ss");
    QString logFileName = QString("logs/NodeGraph_%1.log").arg(timestamp);

    // Redirect Qt debug output to file
    static QFile debugFile(logFileName);
    debugFile.open(QIODevice::WriteOnly | QIODevice::Append);
    static QTextStream stream(&debugFile);

    qInstallMessageHandler([](QtMsgType type, const QMessageLogContext &context, const QString &msg) {
        QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
        QString typeStr;

        switch (type) {
        case QtDebugMsg:
            typeStr = "DEBUG";
            break;
        case QtInfoMsg:
            typeStr = "INFO ";
            break;
        case QtWarningMsg:
            typeStr = "WARN ";
            break;
        case QtCriticalMsg:
            typeStr = "ERROR";
            break;
        case QtFatalMsg:
            typeStr = "FATAL";
            break;
        }

        QString logEntry = QString("[%1] %2: %3").arg(timestamp, typeStr, msg);

        // Write to log file
        stream << logEntry << Qt::endl;
        stream.flush();  // Ensure immediate write
    });

    qDebug() << "=== NodeGraph Application Started ===";
    qDebug() << "Log file:" << logFileName;
    qDebug() << "Timestamp:" << QDateTime::currentDateTime().toString();
}
```

### main.cpp Integration

```cpp
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QDebug>
#include <QUuid>
#include "window.h"

void setupLogging();  // Defined above

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // Setup file logging FIRST (before any qDebug calls)
    setupLogging();

    // Session identification for log correlation
    QString sessionId = QUuid::createUuid().toString(QUuid::WithoutBraces).left(8);
    qDebug() << "Session:" << sessionId;

    // Log command line arguments
    qDebug() << "Command line arguments:";
    for (int i = 0; i < argc; i++) {
        qDebug() << QString("  [%1]: %2").arg(i).arg(argv[i]);
    }

    // Create main window
    Window window;
    window.show();

    int result = app.exec();

    // Log shutdown
    qDebug() << "=== NodeGraph Application Ending ===";
    qDebug() << "Session:" << sessionId << "terminated";

    return result;
}
```

## Example Log Output

```log
[2025-10-09 14:23:44.567] DEBUG: === NodeGraph Application Started ===
[2025-10-09 14:23:44.568] DEBUG: Log file: logs/NodeGraph_2025-10-09_14-23-44.log
[2025-10-09 14:23:44.569] DEBUG: Timestamp: 2025-10-09 14:23:44
[2025-10-09 14:23:44.570] DEBUG: Session: a1b2c3d4
[2025-10-09 14:23:44.571] DEBUG: Command line arguments:
[2025-10-09 14:23:44.572] DEBUG:   [0]: ./NodeGraph
[2025-10-09 14:23:44.800] DEBUG: JavaScriptEngine: Simple JavaScript engine initialized
[2025-10-09 14:23:44.801] DEBUG: GraphFactory initialized with scene and XML document
[2025-10-09 14:23:44.950] DEBUG: NodePalette: Starting population of 5 node templates
[2025-10-09 14:23:45.000] DEBUG: ✓ NodePalette: Populated with 5 socket configuration templates
[2025-10-09 14:23:50.123] DEBUG: Socket right-clicked: index: 0 role: Output
[2025-10-09 14:23:50.124] DEBUG: GHOST: Started from socket 0 ( Output )
[2025-10-09 14:23:51.456] DEBUG: GHOST: ✓ Created edge 0 → 0
[2025-10-09 14:23:51.457] DEBUG: GHOST: Cancelled
[2025-10-09 14:24:00.000] DEBUG: === NodeGraph Application Ending ===
[2025-10-09 14:24:00.001] DEBUG: Session: a1b2c3d4 terminated
```

## Using Logs for Coverage Analysis

### 1. Run App with Logging
```bash
$ ./NodeGraph
# Log created: logs/NodeGraph_2025-10-09_14-23-44.log
```

### 2. Perform Interactive Test
- Create nodes
- Connect with ghost edge
- Save file
- Close app

### 3. Analyze Log File
```bash
$ grep "GHOST" logs/NodeGraph_2025-10-09_14-23-44.log
[2025-10-09 14:23:50.124] DEBUG: GHOST: Started from socket 0
[2025-10-09 14:23:51.456] DEBUG: GHOST: ✓ Created edge 0 → 0
[2025-10-09 14:23:51.457] DEBUG: GHOST: Cancelled
```

### 4. Correlate with Source Code
- If "GHOST: Started" appears → `scene.cpp:startGhostEdge()` was called ✓
- If "GHOST: ✓ Created" appears → `scene.cpp:finishGhostEdge()` was called ✓
- If "GHOST: Cancelled" appears → `scene.cpp:cancelGhostEdge()` was called ✓

### 5. Find Missing Coverage
```bash
# Search for LOG statements that never appeared in log
$ grep -h "qDebug.*GHOST" ghost_edge.cpp
# (no output = ghost_edge.cpp has no logging = 0% visible coverage)
```

## Enhancements for Testing

### Add to ghost_edge.cpp
```cpp
#include <QDebug>

GhostEdge::GhostEdge(QGraphicsItem* parent)
    : QGraphicsPathItem(parent)
{
    qDebug() << "GhostEdge: constructor";
    // ...
}

void GhostEdge::setPath(const QPainterPath& path)
{
    qDebug() << "GhostEdge: setPath - points:" << path.elementCount();
    // ...
}

void GhostEdge::paint(QPainter* painter, ...)
{
    static bool firstPaint = true;
    if (firstPaint) {
        qDebug() << "GhostEdge: paint called (rendering)";
        firstPaint = false;
    }
    // ...
}

GhostEdge::~GhostEdge()
{
    qDebug() << "GhostEdge: destructor";
}
```

### Then Check Log
```bash
$ grep "GhostEdge" logs/NodeGraph_2025-10-09_14-23-44.log
[...] DEBUG: GhostEdge: constructor
[...] DEBUG: GhostEdge: setPath - points: 4
[...] DEBUG: GhostEdge: paint called (rendering)
[...] DEBUG: GhostEdge: destructor
```

If you see these 4 messages → ghost_edge.cpp is being exercised! ✓

## Recommendation

**Restore this logging system for test coverage work:**

1. **Immediate benefit:** See exactly what code runs during interactive tests
2. **Debugging:** Understand test failures with detailed traces
3. **Coverage verification:** Correlate logs with source code
4. **Regression testing:** Compare log outputs between runs

**Implementation:**
- Add `setupLogging()` function to current `main.cpp`
- Call before Window creation
- Add qDebug() to ghost_edge.cpp
- Run tests and analyze log files

---

## Next Steps

Want me to:
1. **Restore the logging system** to current main.cpp?
2. **Add logging** to ghost_edge.cpp?
3. **Create log analysis scripts** to correlate with coverage?
