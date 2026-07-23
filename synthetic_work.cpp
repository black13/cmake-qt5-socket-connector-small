#include "synthetic_work.h"

#include <QCryptographicHash>
#include <QDebug>
#include <QElapsedTimer>
#include <QThread>
#include <QtMath>

namespace {

QString readTask(const QVariantMap& request)
{
    return request.value(QStringLiteral("task"), QStringLiteral("noop")).toString().toLower();
}

// NOTE: helpers receive the started timer and sample elapsed() at the END,
// after the work has actually run (previously durationMs was sampled at
// argument-evaluation time - before the work - so it always reported ~0).

QVariantMap runNoop(const QElapsedTimer& timer)
{
    QVariantMap result;
    result.insert(QStringLiteral("status"), QStringLiteral("ok"));
    result.insert(QStringLiteral("result"), QStringLiteral("noop"));
    result.insert(QStringLiteral("durationMs"), timer.elapsed());
    return result;
}

QVariantMap runLoop(const QVariantMap& request, const QElapsedTimer& timer)
{
    int iterations = request.value(QStringLiteral("iterations"), 100000).toInt();
    // Clamp: scripts run synchronously on the UI thread - unbounded loops
    // freeze the app (the watchdog only covers the JS engine, not C++ work).
    if (iterations > 10000000) {
        qWarning() << "SyntheticWork: iterations clamped from" << iterations << "to" << 10000000;
        iterations = 10000000;
    }
    double accumulator = 0.0;
    for (int i = 0; i < iterations; ++i) {
        accumulator += qSin(i) * qCos(i / 3.0);
    }
    QVariantMap result;
    result.insert(QStringLiteral("status"), QStringLiteral("ok"));
    result.insert(QStringLiteral("result"), accumulator);
    result.insert(QStringLiteral("iterations"), iterations);
    result.insert(QStringLiteral("durationMs"), timer.elapsed());
    return result;
}

QVariantMap runHash(const QVariantMap& request, const QElapsedTimer& timer)
{
    QByteArray payload = request.value(QStringLiteral("payload")).toByteArray();
    if (payload.isEmpty()) {
        payload = QByteArrayLiteral("default-payload");
    }
    QByteArray hash = QCryptographicHash::hash(payload, QCryptographicHash::Sha256);
    QVariantMap result;
    result.insert(QStringLiteral("status"), QStringLiteral("ok"));
    result.insert(QStringLiteral("result"), QString::fromUtf8(hash.toHex()));
    result.insert(QStringLiteral("durationMs"), timer.elapsed());
    return result;
}

QVariantMap runDelay(const QVariantMap& request, const QElapsedTimer& timer)
{
    int delayMs = request.value(QStringLiteral("delayMs"), 10).toInt();
    if (delayMs < 0) {
        delayMs = 0;
    }
    // Clamp: QThread::msleep blocks the UI thread - cap at 5 seconds.
    if (delayMs > 5000) {
        qWarning() << "SyntheticWork: delayMs clamped from" << delayMs << "to" << 5000;
        delayMs = 5000;
    }
    QThread::msleep(static_cast<unsigned long>(delayMs));
    QVariantMap result;
    result.insert(QStringLiteral("status"), QStringLiteral("ok"));
    result.insert(QStringLiteral("result"), QStringLiteral("delay"));
    result.insert(QStringLiteral("delayMs"), delayMs);
    result.insert(QStringLiteral("durationMs"), timer.elapsed()); // measured, not echoed
    return result;
}

} // namespace

QVariantMap SyntheticWork::run(const QVariantMap& request)
{
    const QString task = readTask(request);
    QElapsedTimer timer;
    timer.start();

    if (task == QStringLiteral("loop")) {
        return runLoop(request, timer);
    }
    if (task == QStringLiteral("hash")) {
        return runHash(request, timer);
    }
    if (task == QStringLiteral("delay")) {
        return runDelay(request, timer);
    }
    return runNoop(timer);
}
