#pragma once

#include <QVariantMap>

/**
 * SyntheticWork - deterministic C++ helper invoked from JavaScript.
 *
 * Scripts call this to perform “heavy” work (hashing, math loops, sleep) in C++,
 * keeping JavaScript orchestration simple while we exercise C++ pipelines.
 */
class SyntheticWork
{
public:
    /**
     * Run a named workload.
     * Recognized keys:
     *   task: "noop" | "loop" | "hash" | "delay"
     *   iterations (loop task)
     *   payload (hash task)
     *   delayMs (delay task)
     */
    static QVariantMap run(const QVariantMap& request);
};
