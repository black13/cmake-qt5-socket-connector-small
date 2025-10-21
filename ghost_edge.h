#pragma once

#include <QGraphicsItem>
#include <QPainterPath>
#include <QPen>
#include <QPainter>

class GhostEdge : public QGraphicsItem {
public:
    explicit GhostEdge(QGraphicsItem* parent = nullptr);
    
    void setPath(const QPainterPath& path);
    QPainterPath path() const { return m_path; }
    
    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;
    
private:
    QPainterPath m_path;
    QRectF m_boundingRect;
};