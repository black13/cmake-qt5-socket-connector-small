#include "ghost_edge_item.h"

GhostEdgeItem::GhostEdgeItem(QGraphicsItem* parent)
    : QGraphicsPathItem(parent)
{
    setZValue(-10); // Behind main content
    setFlag(QGraphicsItem::ItemIsSelectable, false);
    setFlag(QGraphicsItem::ItemIsMovable, false);
    setAcceptHoverEvents(false);
    setAcceptedMouseButtons(Qt::NoButton);
}

void GhostEdgeItem::setAdjustedPath(const QPainterPath& path)
{
    prepareGeometryChange();
    m_path = path;
    m_boundingRect = m_path.boundingRect().adjusted(-4, -4, 4, 4); // Minimum inflation
    setPath(m_path);
}

QRectF GhostEdgeItem::boundingRect() const
{
    return m_boundingRect;
}

void GhostEdgeItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(option)
    Q_UNUSED(widget)

    painter->setRenderHint(QPainter::Antialiasing);

    QPen pen(QColor(0, 255, 0, 150), 3, Qt::DashLine);
    pen.setCapStyle(Qt::RoundCap);
    pen.setJoinStyle(Qt::RoundJoin);
    pen.setDashPattern({8, 4});

    painter->setPen(pen);
    painter->setBrush(Qt::NoBrush);
    painter->drawPath(m_path);
}