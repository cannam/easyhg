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

#include "dateitem.h"

#include "debug.h"

#include <QPainter>
#include <QBrush>
#include <QFont>
#include <QGraphicsSceneMouseEvent>

DateItem::DateItem() :
    m_minrow(0), m_maxrow(0),
    m_mincol(0), m_maxcol(0),
    m_even(false)
{
}

void
DateItem::setRows(int minrow, int n)
{
    m_minrow = minrow;
    m_maxrow = minrow + n - 1;
    setY(m_minrow * 90);
}

void
DateItem::setCols(int mincol, int n)
{
    m_mincol = mincol;
    m_maxcol = mincol + n - 1;
    setX(m_mincol * 100);
}

void
DateItem::mousePressEvent(QGraphicsSceneMouseEvent *e)
{
    DEBUG << "DateItem::mousePressEvent" << endl;
    if (e->button() == Qt::LeftButton) {
        emit clicked();
    }
    e->ignore();
}

QRectF
DateItem::boundingRect() const
{
    return QRectF(-75, -25,
		  (m_maxcol - m_mincol + 1) * 100 + 100,
		  (m_maxrow - m_minrow + 1) * 90).normalized();
}

void
DateItem::paint(QPainter *paint, const QStyleOptionGraphicsItem *opt, QWidget *w)
{
    QBrush brush;

    if (m_even) {
	QColor c(QColor::fromRgb(240, 240, 240));
	brush = QBrush(c);
    } else {
	QColor c(QColor::fromRgb(250, 250, 250));
	brush = QBrush(c);
    }

    paint->fillRect(boundingRect(), brush);

    paint->save();
    QFont f(paint->font());
    f.setBold(true);
    paint->setFont(f);
    paint->drawText(-70, -10, m_dateString);
    paint->restore();
}


