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

#include "panned.h"
#include "debug.h"

#include <QScrollBar>
#include <QWheelEvent>

#include <iostream>

Panned::Panned()
{
    setRenderHints(QPainter::Antialiasing |
                   QPainter::TextAntialiasing);
}

void
Panned::resizeEvent(QResizeEvent *ev)
{
    DEBUG << "Panned::resizeEvent()" << endl;

    QPointF nearpt = mapToScene(0, 0);
    QPointF farpt = mapToScene(width(), height());
    QSizeF sz(farpt.x()-nearpt.x(), farpt.y()-nearpt.y());
    QRectF pr(nearpt, sz);

    QGraphicsView::resizeEvent(ev);

    if (pr != m_pannedRect) {
        DEBUG << "Panned: setting panned rect to " << pr << endl;
        m_pannedRect = pr;
        emit pannedRectChanged(pr);
    }
}

void
Panned::paintEvent(QPaintEvent *e)
{
    QGraphicsView::paintEvent(e);
}

void
Panned::drawForeground(QPainter *paint, const QRectF &)
{
    QPointF nearpt = mapToScene(0, 0);
    QPointF farpt = mapToScene(width(), height());
    QSizeF sz(farpt.x()-nearpt.x(), farpt.y()-nearpt.y());
    QRectF pr(nearpt, sz);

    if (pr != m_pannedRect) {
        if (pr.x() != m_pannedRect.x()) emit pannedContentsScrolled();
        m_pannedRect = pr;
        emit pannedRectChanged(pr);
    }
}

void
Panned::zoomIn()
{
    QMatrix m = matrix();
    m.scale(1.0 / 1.1, 1.0 / 1.1);
    setMatrix(m);
}

void
Panned::zoomOut()
{
    QMatrix m = matrix();
    m.scale(1.1, 1.1);
    setMatrix(m);
}

void
Panned::slotSetPannedRect(QRectF pr)
{
    centerOn(pr.center());
//	setSceneRect(pr);
//	m_pannedRect = pr;
}

void
Panned::wheelEvent(QWheelEvent *ev)
{
    if (ev->modifiers() & Qt::ControlModifier) {
        int d = ev->delta();
        if (d > 0) {
            while (d > 0) {
                zoomOut();
                d -= 120;
            }
        } else {
            while (d < 0) {
                zoomIn();
                d += 120;
            }
        }
    } else {
        emit wheelEventReceived(ev);
        QGraphicsView::wheelEvent(ev);
    }
}

void
Panned::slotEmulateWheelEvent(QWheelEvent *ev)
{
    QGraphicsView::wheelEvent(ev);
}

void
Panned::leaveEvent(QEvent *)
{
    emit mouseLeaves();
}
