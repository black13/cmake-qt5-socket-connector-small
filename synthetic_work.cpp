#include "synthetic_work.h"

#include <QCryptographicHash>
#include <QElapsedTimer>
#include <QThread>
#include <QtMath>

namespace {

QString readTask(const QVariantMap& request)
{
    return request.value(QStringLiteral("task"), QStringLiteral("noop")).toString().toLower();
}

QVariantMap runNoop(qint64 durationMs)
{
    QVariantMap result;
    result.insert(QStringLiteral("status"), QStringLiteral("ok"));
    result.insert(QStringLiteral("result"), QStringLiteral("noop"));
    result.insert(QStringLiteral("durationMs"), durationMs);
    return result;
}

QVariantMap runLoop(const QVariantMap& request, qint64 durationMs)
{
    int iterations = request.value(QStringLiteral("iterations"), 100000).toInt();
    double accumulator = 0.0;
    for (int i = 0; i < iterations; ++i) {
        accumulator += qSin(i) * qCos(i / 3.0);
    }
    QVariantMap result;
    result.insert(QStringLiteral("status"), QStringLiteral("ok"));
    result.insert(QStringLiteral("result"), accumulator);
    result.insert(QStringLiteral("iterations"), iterations);
    result.insert(QStringLiteral("durationMs"), durationMs);
    return result;
}

QVariantMap runHash(const QVariantMap& request, qint64 durationMs)
{
    QByteArray payload = request.value(QStringLiteral("payload")).toByteArray();
    if (payload.isEmpty()) {
        payload = QByteArrayLiteral("default-payload");
    }
    QByteArray hash = QCryptographicHash::hash(payload, QCryptographicHash::Sha256);
    QVariantMap result;
    result.insert(QStringLiteral("status"), QStringLiteral("ok"));
    result.insert(QStringLiteral("result"), QString::fromUtf8(hash.toHex()));
    result.insert(QStringLiteral("durationMs"), durationMs);
    return result;
}

QVariantMap runDelay(const QVariantMap& request, qint64 /*durationMs*/)
{
    int delayMs = request.value(QStringLiteral("delayMs"), 10).toInt();
    if (delayMs < 0) {
        delayMs = 0;
    }
    QThread::msleep(static_cast<unsigned long>(delayMs));
    QVariantMap result;
    result.insert(QStringLiteral("status"), QStringLiteral("ok"));
    result.insert(QStringLiteral("result"), QStringLiteral("delay"));
    result.insert(QStringLiteral("delayMs"), delayMs);
    result.insert(QStringLiteral("durationMs"), delayMs);
    return result;
}

} // namespace

QVariantMap SyntheticWork::run(const QVariantMap& request)
{
    const QString task = readTask(request);
    QElapsedTimer timer;
    timer.start();

    if (task == QStringLiteral("loop")) {
        return runLoop(request, timer.elapsed());
    }
    if (task == QStringLiteral("hash")) {
        return runHash(request, timer.elapsed());
    }
    if (task == QStringLiteral("delay")) {
        return runDelay(request, timer.elapsed());
    }
    return runNoop(timer.elapsed());
}
