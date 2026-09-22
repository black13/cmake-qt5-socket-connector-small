#pragma once

#include <QLoggingCategory>

/**
 * nodegraph_logging.h - opt-in diagnostics
 *
 * Hot-path traces (per-node/socket/edge creation, scene bookkeeping, status
 * refreshes) use this category. It is disabled by default because a 400-node
 * workload otherwise writes ~2 MB of log lines and logging dominates the
 * mutation cost.
 *
 * Enable when debugging:
 *   QT_LOGGING_RULES="nodegraph.verbose.debug=true" NodeGraph.exe
 * (main() sets the default rule; the environment variable overrides it.)
 */
Q_DECLARE_LOGGING_CATEGORY(ngVerbose)
