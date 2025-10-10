#include <QApplication>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QDebug>
#include <QUuid>
#include "window.h"

/**
 * setupLogging - Redirect all qDebug() output to timestamped log files
 *
 * Creates log files in logs/ directory with format: NodeGraph_YYYY-MM-DD_hh-mm-ss.log
 * Includes timestamps, severity levels, and session tracking for debugging and test analysis
 */
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
        Q_UNUSED(context);

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
        stream.flush();  // Ensure immediate write for test debugging

        // Also print to console for real-time monitoring
        fprintf(stderr, "%s\n", logEntry.toUtf8().constData());
    });

    qDebug() << "=== NodeGraph Application Started ===";
    qDebug() << "Log file:" << logFileName;
    qDebug() << "Timestamp:" << QDateTime::currentDateTime().toString();
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // Setup file logging FIRST (before any qDebug calls)
    setupLogging();

    // Session identification for log correlation (helpful for test analysis)
    QString sessionId = QUuid::createUuid().toString(QUuid::WithoutBraces).left(8);
    qDebug() << "Session:" << sessionId;

    // Log command line arguments for debugging
    qDebug() << "Command line arguments:";
    for (int i = 0; i < argc; i++) {
        qDebug() << QString("  [%1]: %2").arg(i).arg(argv[i]);
    }

    // main() stays a pure bootstrapper — no XML, no factory, no I/O here.
    Window window;
    window.show();

    int result = app.exec();

    // Log shutdown
    qDebug() << "=== NodeGraph Application Ending ===";
    qDebug() << "Session:" << sessionId << "terminated";

    return result;
}
