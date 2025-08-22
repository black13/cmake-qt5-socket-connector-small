#pragma once

#include <QJSEngine>
#include <QJSValue>
#include <QString>
#include <QElapsedTimer>
#include <QTimer>
#include <QObject>
#include <QMutex>

/**
 * ScriptExecutor - Safe JavaScript execution with error isolation
 * 
 * Provides comprehensive safety measures for JavaScript execution:
 * - Execution timeouts to prevent infinite loops
 * - Error isolation to prevent script crashes from affecting the application
 * - Resource monitoring and limits
 * - Detailed execution metrics and error reporting
 * - Recovery mechanisms for failed scripts
 */
class ScriptExecutor : public QObject
{
    Q_OBJECT

public:
    struct ExecutionResult {
        bool success;
        QJSValue result;
        QString error;
        qint64 executionTimeMs;
        bool timedOut;
        bool crashed;
        QString executionPhase;
        
        // Default constructor
        ExecutionResult() 
            : success(false), timedOut(false), crashed(false), executionTimeMs(0) {}
    };
    
    struct ExecutionOptions {
        int timeoutMs;
        bool enableConsole;
        bool enableDebugging;
        int maxRecursionDepth;
        
        ExecutionOptions() 
            : timeoutMs(5000)
            , enableConsole(true)
            , enableDebugging(false)
            , maxRecursionDepth(100)
        {}
    };

    explicit ScriptExecutor(QObject* parent = nullptr);
    ~ScriptExecutor();
    
    // Safe execution methods
    static ExecutionResult safeExecute(QJSEngine* engine, const QString& script, 
                                     const ExecutionOptions& options = ExecutionOptions());
    
    // Instance-based execution with more control
    ExecutionResult execute(QJSEngine* engine, const QString& script, 
                          const ExecutionOptions& options = ExecutionOptions());
    
    // Script validation
    static bool validateScript(const QString& script, QString& errorMessage);
    
    // Recovery methods
    static bool recoverEngine(QJSEngine* engine);
    
    // Utility methods
    static QString formatExecutionResult(const ExecutionResult& result);
    static bool isScriptSafe(const QString& script);

signals:
    void scriptStarted(const QString& script);
    void scriptCompleted(const ExecutionResult& result);
    void scriptTimeout(int timeoutMs);
    void scriptError(const QString& error);

private slots:
    void handleTimeout();

private:
    QTimer* m_timeoutTimer;
    QMutex m_executionMutex;
    bool m_isExecuting;
    QString m_currentScript;
    
    // Internal execution phases
    ExecutionResult executeWithIsolation(QJSEngine* engine, const QString& script, 
                                        const ExecutionOptions& options);
    bool setupExecution(QJSEngine* engine, const ExecutionOptions& options);
    void cleanupExecution();
    
    // Error detection and recovery
    bool detectInfiniteLoop(const QString& script);
    bool detectMaliciousCode(const QString& script);
    void logExecutionMetrics(const ExecutionResult& result);
};