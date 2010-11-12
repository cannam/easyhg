

#include "connectionitem.h"

#include "changesetitem.h"
#include "changeset.h"
#include "colourset.h"

#include <QPainter>

QRectF
ConnectionItem::boundingRect() const
{
    if (!m_parent || !m_child) return QRectF();
    float xscale = 100;
    float yscale = 90;
    float size = 50;
    return QRectF(xscale * m_child->column() + size/2 - 2,
		  yscale * m_child->row() + size - 2,
		  xscale * m_parent->column() - xscale * m_child->column() + 4,
		  yscale * m_parent->row() - yscale * m_child->row() - size + 4)
	.normalized();
}

void
ConnectionItem::paint(QPainter *paint, const QStyleOptionGraphicsItem *, QWidget *)
{
    QPainterPath p;

    paint->save();

    ColourSet *colourSet = ColourSet::instance();
    QColor branchColour = colourSet->getColourFor(m_child->getChangeset()->branch());

    QTransform t = paint->worldTransform();
    float scale = std::min(t.m11(), t.m22());
    if (scale < 0.1) {
	paint->setPen(QPen(branchColour, 0));
    } else {
	paint->setPen(QPen(branchColour, 2));
    }

    float xscale = 100;

    float yscale = 90;
    float size = 50;
    float ygap = yscale - size;

    int c_col = m_child->column(), c_row = m_child->row();
    int p_col = m_parent->column(), p_row = m_parent->row();

    float c_x = xscale * c_col + size/2;
    float p_x = xscale * p_col + size/2;

    p.moveTo(c_x, yscale * c_row + size);

    if (p_col == c_col) {

	p.lineTo(p_x, yscale * p_row);

    } else if (m_type == Split || m_type == Normal) {

	// place the bulk of the line on the child (i.e. branch) row

	if (abs(p_row - c_row) > 1) {
	    p.lineTo(c_x, yscale * p_row - ygap);
	}

	p.cubicTo(c_x, yscale * p_row,
		  p_x, yscale * p_row - ygap,
		  p_x, yscale * p_row);

    } else if (m_type == Merge) {

	// place bulk of the line on the parent row

	p.cubicTo(c_x, yscale * c_row + size + ygap,
		  p_x, yscale * c_row + size,
		  p_x, yscale * c_row + size + ygap);

	if (abs(p_row - c_row) > 1) {
	    p.lineTo(p_x, yscale * p_row);
	}
    }

    paint->drawPath(p);
    paint->restore();
}


