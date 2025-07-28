#include "ghost_edge.h"

GhostEdge::GhostEdge(QGraphicsItem* parent)
    : QGraphicsItem(parent)
{
    setFlag(QGraphicsItem::ItemIsSelectable, false);
    setFlag(QGraphicsItem::ItemIsMovable, false);
    setFlag(QGraphicsItem::ItemIsFocusable, false);
    setAcceptHoverEvents(false);
    setAcceptedMouseButtons(Qt::NoButton);
    setAcceptTouchEvents(false);
    setCacheMode(QGraphicsItem::NoCache);
    setZValue(-10);
}

void GhostEdge::setPath(const QPainterPath& path)
{
    prepareGeometryChange();
    m_path = path;
    m_boundingRect = path.boundingRect().adjusted(-2, -2, 2, 2);
}

QRectF GhostEdge::boundingRect() const
{
    return m_boundingRect;
}

void GhostEdge::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*)
{
    painter->setRenderHint(QPainter::Antialiasing);
    QPen pen(QColor(0, 255, 0, 150), 2, Qt::DashLine);
    pen.setCapStyle(Qt::RoundCap);
    painter->setPen(pen);
    painter->setBrush(Qt::NoBrush);
    painter->drawPath(m_path);
}