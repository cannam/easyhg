

#include "connectionitem.h"

#include "changesetitem.h"

#include <QPainter>

QRectF
ConnectionItem::boundingRect() const
{
    if (!m_parent || !m_child) return QRectF();
    float scale = 100;
    float size = 50;
    return QRectF(scale * m_child->column() + size/2 - 2,
		  scale * m_child->row() + size - 2,
		  scale * m_parent->column() - scale * m_child->column() + 4,
		  scale * m_parent->row() - scale * m_child->row() - size + 4)
	.normalized();
}

void
ConnectionItem::paint(QPainter *paint, const QStyleOptionGraphicsItem *, QWidget *)
{
    QPainterPath p;
    float scale = 100;
    float size = 50;
    p.moveTo(scale * m_child->column() + size/2,
	     scale * m_child->row() + size);
    if (m_parent->column() == m_child->column()) {
	p.lineTo(scale * m_parent->column() + size/2,
		 scale * m_parent->row());
    } else {
	p.cubicTo(scale * m_child->column() + size/2,
		  scale * m_child->row() + size + size,
		  scale * m_parent->column() + size/2,
		  scale * m_child->row() + size,
		  scale * m_parent->column() + size/2,
		  scale * m_child->row() + scale);
	if (abs(m_parent->row() - m_child->row()) > 1) {
	    p.lineTo(scale * m_parent->column() + size/2,
		     scale * m_parent->row());
	}
    }
    paint->drawPath(p);
}


