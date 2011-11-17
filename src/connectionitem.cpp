/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    EasyMercurial

    Based on HgExplorer by Jari Korhonen
    Copyright (c) 2010 Jari Korhonen
    Copyright (c) 2011 Chris Cannam
    Copyright (c) 2011 Queen Mary, University of London
    
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
#include "textabbrev.h"

#include <QPainter>
#include <QFont>

QRectF
ConnectionItem::boundingRect() const
{
    if (!(m_child || m_uncommitted)) return QRectF();
    float xscale = 100;
    float yscale = 90;
    float size = 50;

    int c_col, c_row;
    if (m_child) {
        c_col = m_child->column(); c_row = m_child->row();
    } else {
        c_col = m_uncommitted->column(); c_row = m_uncommitted->row();
    }

    int p_col, p_row;
    if (m_parent) {
        p_col = m_parent->column(); p_row = m_parent->row();
    } else {
        p_col = c_col - 1; p_row = c_row + 1;
    }

    return QRectF(xscale * c_col + size/2 - 2,
		  yscale * c_row + size - 22,
		  xscale * p_col - xscale * c_col + 6,
		  yscale * p_row - yscale * c_row - size + 44)
	.normalized();
}

void
ConnectionItem::paint(QPainter *paint, const QStyleOptionGraphicsItem *, QWidget *)
{
    if (!(m_child || m_uncommitted)) return;
    QPainterPath p;

    paint->save();

    int alpha = 255;
    if (m_child && m_child->isClosed()) alpha = 90;

    ColourSet *colourSet = ColourSet::instance();
    QString branch;
    if (m_child) branch = m_child->getChangeset()->branch();
    else branch = m_uncommitted->branch();
    QColor branchColour = colourSet->getColourFor(branch);

    branchColour.setAlpha(alpha);

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
    float ygap = yscale - size - 2;

    int c_col, c_row;
    if (m_child) {
        c_col = m_child->column(); c_row = m_child->row();
    } else {
        c_col = m_uncommitted->column(); c_row = m_uncommitted->row();
    }

    int p_col, p_row;
    if (m_parent) {
        p_col = m_parent->column(); p_row = m_parent->row();
    } else {
        p_col = c_col - 1; p_row = c_row + 1;
    }

    float c_x = xscale * c_col + size/2;
    float p_x = xscale * p_col + size/2;

    // ensure line reaches the box, even if it's in a small height --
    // doesn't matter if we overshoot as the box is opaque and has a
    // greater Z value
    p.moveTo(c_x, yscale * c_row + size - 20);

    p.lineTo(c_x, yscale * c_row + size);

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

    if (m_parent) {

        // ensure line reaches the node -- again doesn't matter if we
        // overshoot
        p.lineTo(p_x, yscale * p_row + 20);

    } else {

        // no parent: merge from closed branch: draw only half the line
        paint->setClipRect(QRectF((c_x + p_x)/2, yscale * c_row + size - 22,
                                  xscale, yscale));
    }
    
    paint->drawPath(p);

    if (!m_parent) {

        // merge from closed branch: draw branch name

        paint->setClipping(false);

        QFont f;
        f.setPixelSize(11);
        f.setBold(true);
        f.setItalic(false);
	paint->setFont(f);

	QString branch = m_mergedBranch;
        if (branch == "") branch = "default";
	int wid = xscale;
	branch = TextAbbrev::abbreviate(branch, QFontMetrics(f), wid);
	paint->drawText((c_x + p_x)/2 - wid - 2,
                        yscale * c_row + size + ygap/2 + 2,
                        branch);
    }
    
    paint->restore();
}


