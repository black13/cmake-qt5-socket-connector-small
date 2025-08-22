#include "script_executor.h"
#include <QDebug>
#include <QRegularExpression>
#include <QCoreApplication>
#include <QMutexLocker>
#include <QThread>

ScriptExecutor::ScriptExecutor(QObject* parent)
    : QObject(parent)
    , m_timeoutTimer(new QTimer(this))
    , m_isExecuting(false)
{
    m_timeoutTimer->setSingleShot(true);
    connect(m_timeoutTimer, &QTimer::timeout, this, &ScriptExecutor::handleTimeout);
}

ScriptExecutor::~ScriptExecutor()
{
    if (m_isExecuting) {
        m_timeoutTimer->stop();
    }
}

ScriptExecutor::ExecutionResult ScriptExecutor::safeExecute(QJSEngine* engine, const QString& script, 
                                                           const ExecutionOptions& options)
{
    ExecutionResult result;
    
    if (!engine) {
        result.error = "ScriptExecutor: Engine is null";
        result.executionPhase = "validation";
        return result;
    }
    
    if (script.isEmpty()) {
        result.error = "ScriptExecutor: Script is empty";
        result.executionPhase = "validation";
        return result;
    }
    
    // Pre-execution validation
    QString validationError;
    if (!validateScript(script, validationError)) {
        result.error = "ScriptExecutor: Script validation failed: " + validationError;
        result.executionPhase = "validation";
        return result;
    }
    
    qDebug() << "ScriptExecutor: Starting safe execution";
    qDebug() << "ScriptExecutor: Script length:" << script.length();
    qDebug() << "ScriptExecutor: Timeout:" << options.timeoutMs << "ms";
    
    QElapsedTimer timer;
    timer.start();
    
    result.executionPhase = "execution";
    
    try {
        // Execute the script with error isolation
        QJSValue jsResult = engine->evaluate(script);
        
        result.executionTimeMs = timer.elapsed();
        
        if (jsResult.isError()) {
            result.success = false;
            result.error = QString("JavaScript Error: %1").arg(jsResult.toString());
            result.executionPhase = "javascript_error";
        } else {
            result.success = true;
            result.result = jsResult;
            result.executionPhase = "completed";
        }
        
        // Check for timeout
        if (result.executionTimeMs > options.timeoutMs) {
            result.timedOut = true;
            result.error = QString("Script execution timed out after %1ms").arg(result.executionTimeMs);
            qWarning() << "ScriptExecutor: Script timed out:" << result.executionTimeMs << "ms";
        }
        
    } catch (const std::exception& e) {
        result.success = false;
        result.crashed = true;
        result.error = QString("C++ Exception during script execution: %1").arg(e.what());
        result.executionTimeMs = timer.elapsed();
        result.executionPhase = "cpp_exception";
        
        qCritical() << "ScriptExecutor: C++ exception:" << e.what();
        
        // Attempt engine recovery
        if (recoverEngine(engine)) {
            qDebug() << "ScriptExecutor: Engine recovery successful";
        } else {
            qCritical() << "ScriptExecutor: Engine recovery failed";
        }
        
    } catch (...) {
        result.success = false;
        result.crashed = true;
        result.error = "Unknown exception during script execution";
        result.executionTimeMs = timer.elapsed();
        result.executionPhase = "unknown_exception";
        
        qCritical() << "ScriptExecutor: Unknown exception";
        recoverEngine(engine);
    }
    
    // Log execution metrics
    if (options.enableDebugging) {
        qDebug() << "ScriptExecutor: Execution completed";
        qDebug() << "ScriptExecutor: Success:" << result.success;
        qDebug() << "ScriptExecutor: Time:" << result.executionTimeMs << "ms";
        qDebug() << "ScriptExecutor: Phase:" << result.executionPhase;
        if (!result.success) {
            qDebug() << "ScriptExecutor: Error:" << result.error;
        }
    }
    
    return result;
}

ScriptExecutor::ExecutionResult ScriptExecutor::execute(QJSEngine* engine, const QString& script, 
                                                       const ExecutionOptions& options)
{
    QMutexLocker locker(&m_executionMutex);
    
    if (m_isExecuting) {
        ExecutionResult result;
        result.error = "ScriptExecutor: Already executing a script";
        result.executionPhase = "busy";
        return result;
    }
    
    m_isExecuting = true;
    m_currentScript = script;
    
    emit scriptStarted(script);
    
    // Setup timeout timer
    if (options.timeoutMs > 0) {
        m_timeoutTimer->start(options.timeoutMs);
    }
    
    ExecutionResult result = executeWithIsolation(engine, script, options);
    
    m_timeoutTimer->stop();
    m_isExecuting = false;
    m_currentScript.clear();
    
    emit scriptCompleted(result);
    
    return result;
}

bool ScriptExecutor::validateScript(const QString& script, QString& errorMessage)
{
    if (script.isEmpty()) {
        errorMessage = "Script is empty";
        return false;
    }
    
    if (script.length() > 1000000) { // 1MB limit
        errorMessage = "Script too large (>1MB)";
        return false;
    }
    
    // Check for potentially dangerous patterns
    QStringList dangerousPatterns = {
        "while\\s*\\(\\s*true\\s*\\)",           // while(true)
        "for\\s*\\(\\s*;\\s*;\\s*\\)",           // for(;;)
        "eval\\s*\\(",                           // eval()
        "Function\\s*\\(",                       // Function()
        "setInterval",                           // setInterval
        "setTimeout"                             // setTimeout
    };
    
    for (const QString& pattern : dangerousPatterns) {
        QRegularExpression regex(pattern, QRegularExpression::CaseInsensitiveOption);
        if (regex.match(script).hasMatch()) {
            errorMessage = QString("Script contains potentially dangerous pattern: %1").arg(pattern);
            qWarning() << "ScriptExecutor: Dangerous pattern detected:" << pattern;
            // Don't fail validation, just warn
        }
    }
    
    // Basic syntax check - try to detect obvious syntax errors
    if (script.count('(') != script.count(')')) {
        errorMessage = "Mismatched parentheses";
        return false;
    }
    
    if (script.count('{') != script.count('}')) {
        errorMessage = "Mismatched braces";
        return false;
    }
    
    if (script.count('[') != script.count(']')) {
        errorMessage = "Mismatched brackets";
        return false;
    }
    
    return true;
}

bool ScriptExecutor::recoverEngine(QJSEngine* engine)
{
    if (!engine) {
        return false;
    }
    
    try {
        qDebug() << "ScriptExecutor: Attempting engine recovery";
        
        // Clear any pending exceptions
        engine->globalObject().setProperty("__recovery_test", QJSValue());
        
        // Test basic functionality
        QJSValue testResult = engine->evaluate("2 + 2");
        if (testResult.isError() || testResult.toInt() != 4) {
            qWarning() << "ScriptExecutor: Engine recovery failed - basic math test failed";
            return false;
        }
        
        qDebug() << "ScriptExecutor: Engine recovery successful";
        return true;
        
    } catch (...) {
        qCritical() << "ScriptExecutor: Engine recovery failed with exception";
        return false;
    }
}

QString ScriptExecutor::formatExecutionResult(const ExecutionResult& result)
{
    QString output;
    QTextStream stream(&output);
    
    stream << "=== Script Execution Result ===\n";
    stream << "Success: " << (result.success ? "YES" : "NO") << "\n";
    stream << "Execution Time: " << result.executionTimeMs << "ms\n";
    stream << "Phase: " << result.executionPhase << "\n";
    
    if (result.timedOut) {
        stream << "Status: TIMED OUT\n";
    } else if (result.crashed) {
        stream << "Status: CRASHED\n";
    } else if (result.success) {
        stream << "Status: SUCCESS\n";
    } else {
        stream << "Status: FAILED\n";
    }
    
    if (!result.error.isEmpty()) {
        stream << "Error: " << result.error << "\n";
    }
    
    if (result.success && !result.result.isUndefined()) {
        stream << "Result: " << result.result.toString() << "\n";
    }
    
    stream << "==============================\n";
    
    return output;
}

bool ScriptExecutor::isScriptSafe(const QString& script)
{
    QString error;
    return validateScript(script, error);
}

void ScriptExecutor::handleTimeout()
{
    qWarning() << "ScriptExecutor: Script execution timed out";
    emit scriptTimeout(m_timeoutTimer->interval());
    
    if (m_isExecuting) {
        qWarning() << "ScriptExecutor: Terminating timed out script";
        // Note: QJSEngine doesn't provide a direct way to interrupt execution
        // This is logged for debugging purposes
    }
}

ScriptExecutor::ExecutionResult ScriptExecutor::executeWithIsolation(QJSEngine* engine, const QString& script, 
                                                                    const ExecutionOptions& options)
{
    // Use the static method for actual execution
    return safeExecute(engine, script, options);
}