#include "ghost_edge.h"
#include <QDebug>

GhostEdge::GhostEdge(QGraphicsItem* parent)
    : QGraphicsItem(parent)
{
    qDebug() << "GhostEdge: constructor - creating visual preview for edge connection";

    setFlag(QGraphicsItem::ItemIsSelectable, false);
    setFlag(QGraphicsItem::ItemIsMovable, false);
    setFlag(QGraphicsItem::ItemIsFocusable, false);
    setAcceptHoverEvents(false);
    setAcceptedMouseButtons(Qt::NoButton);
    setAcceptTouchEvents(false);
    setCacheMode(QGraphicsItem::NoCache);
    setZValue(-10);
}

GhostEdge::~GhostEdge()
{
    qDebug() << "GhostEdge: destructor - cleaning up visual preview";
}

void GhostEdge::setPath(const QPainterPath& path)
{
    qDebug() << "GhostEdge: setPath - updating preview path with" << path.elementCount() << "elements";

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
    // Log only first paint to avoid log spam during mouse movement
    static bool firstPaint = true;
    if (firstPaint) {
        qDebug() << "GhostEdge: paint - rendering visual preview (green dashed line)";
        firstPaint = false;
    }

    painter->setRenderHint(QPainter::Antialiasing);
    QPen pen(QColor(0, 255, 0, 150), 2, Qt::DashLine);
    pen.setCapStyle(Qt::RoundCap);
    painter->setPen(pen);
    painter->setBrush(Qt::NoBrush);
    painter->drawPath(m_path);
}