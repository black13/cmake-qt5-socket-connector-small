#pragma once

#include <QGraphicsPathItem>
#include <QPen>
#include <QPainter>

class GhostEdgeItem : public QGraphicsPathItem {
public:
    GhostEdgeItem(QGraphicsItem* parent = nullptr);

    void setAdjustedPath(const QPainterPath& path);
    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

private:
    QPainterPath m_path;
    QRectF m_boundingRect;
};