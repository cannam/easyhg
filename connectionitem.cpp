/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    EasyMercurial

    Based on HgExplorer by Jari Korhonen
    Copyright (c) 2010 Jari Korhonen
    Copyright (c) 2010 Chris Cannam
    Copyright (c) 2010 Queen Mary, University of London
    
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "connectionitem.h"
#include "uncommitteditem.h"

#include "changesetitem.h"
#include "changeset.h"
#include "colourset.h"

#include <QPainter>

QRectF
ConnectionItem::boundingRect() const
{
    if (!m_parent || !(m_child || m_uncommitted)) return QRectF();
    float xscale = 100;
    float yscale = 90;
    float size = 50;

    int p_col = m_parent->column(), p_row = m_parent->row();
    int c_col, c_row;
    if (m_child) {
        c_col = m_child->column(); c_row = m_child->row();
    } else {
        c_col = m_uncommitted->column(); c_row = m_uncommitted->row();
    }

    return QRectF(xscale * c_col + size/2 - 2,
		  yscale * c_row + size - 2,
		  xscale * p_col - xscale * c_col + 4,
		  yscale * p_row - yscale * c_row - size + 4)
	.normalized();
}

void
ConnectionItem::paint(QPainter *paint, const QStyleOptionGraphicsItem *, QWidget *)
{
    if (!m_parent || !(m_child || m_uncommitted)) return;
    QPainterPath p;

    paint->save();

    ColourSet *colourSet = ColourSet::instance();
    QString branch;
    if (m_child) branch = m_child->getChangeset()->branch();
    else branch = m_uncommitted->branch();
    QColor branchColour = colourSet->getColourFor(branch);

    Qt::PenStyle ls = Qt::SolidLine;
    if (!m_child) ls = Qt::DashLine;

    QTransform t = paint->worldTransform();
    float scale = std::min(t.m11(), t.m22());
    if (scale < 0.2) {
	paint->setPen(QPen(branchColour, 0, ls));
    } else {
	paint->setPen(QPen(branchColour, 2, ls));
    }

    float xscale = 100;

    float yscale = 90;
    float size = 50;
    float ygap = yscale - size;

    int p_col = m_parent->column(), p_row = m_parent->row();
    int c_col, c_row;
    if (m_child) {
        c_col = m_child->column(); c_row = m_child->row();
    } else {
        c_col = m_uncommitted->column(); c_row = m_uncommitted->row();
    }

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


