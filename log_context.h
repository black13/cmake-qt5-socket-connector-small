#pragma once

#include <QDebug>
#include <QUuid>
#include <QString>

/**
 * Logging context helpers for consistent, informative debug output
 *
 * Provides macros that automatically include class name, object ID, and function context.
 * Reduces repetition and ensures all logs have consistent formatting.
 *
 * Usage:
 *   LOG_NODE() << "additional message";        // Requires m_id member
 *   LOG_EDGE() << "additional message";        // Requires m_id member
 *   LOG_SOCKET() << "index" << m_index;        // Requires m_index member
 *   LOG_CONTEXT("ClassName") << "message";     // Generic context
 */

// Helper to format UUID consistently (8-char abbreviated form)
inline QString formatUuid(const QUuid& uuid) {
    return uuid.toString(QUuid::WithoutBraces).left(8);
}

// Logging macros with automatic context
#define LOG_NODE() \
    qDebug() << "[Node:" << formatUuid(m_id) << "]" << Q_FUNC_INFO

#define LOG_EDGE() \
    qDebug() << "[Edge:" << formatUuid(m_id) << "]" << Q_FUNC_INFO

#define LOG_SOCKET() \
    qDebug() << "[Socket:" << m_role << "#" << m_index << "]" << Q_FUNC_INFO

#define LOG_SCENE() \
    qDebug() << "[Scene]" << Q_FUNC_INFO

#define LOG_FACTORY() \
    qDebug() << "[GraphFactory]" << Q_FUNC_INFO

#define LOG_CONTEXT(className) \
    qDebug() << "[" << className << "]" << Q_FUNC_INFO

// Simple variants without function name (for high-frequency logs)
#define LOG_NODE_SIMPLE() \
    qDebug() << "[Node:" << formatUuid(m_id) << "]"

#define LOG_EDGE_SIMPLE() \
    qDebug() << "[Edge:" << formatUuid(m_id) << "]"

#define LOG_SOCKET_SIMPLE() \
    qDebug() << "[Socket:" << m_role << "#" << m_index << "]"
