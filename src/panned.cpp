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

#include "panned.h"
#include "debug.h"

#include <QScrollBar>
#include <QWheelEvent>
#include <QTimer>

#include <cmath>

#include <iostream>

Panned::Panned() :
    m_dragging(false)
{
    m_dragTimer = new QTimer(this);
    m_dragTimerMs = 50;
    connect(m_dragTimer, SIGNAL(timeout()), this, SLOT(dragTimerTimeout()));
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
        centerOn(pr.center());
        emit pannedRectChanged(pr);
    }
}

void
Panned::setScene(QGraphicsScene *s)
{
    if (!scene()) {
        QGraphicsView::setScene(s);
        return;
    }

    QPointF nearpt = mapToScene(0, 0);
    QPointF farpt = mapToScene(width(), height());
    QSizeF sz(farpt.x()-nearpt.x(), farpt.y()-nearpt.y());
    QRectF pr(nearpt, sz);

    QGraphicsView::setScene(s);

    DEBUG << "Panned::setScene: pr = " << pr << ", sceneRect = " << sceneRect() << endl;

    if (scene() && sceneRect().intersects(pr)) {
        DEBUG << "Panned::setScene: restoring old rect " << pr << endl;
        m_pannedRect = pr;
        centerOn(pr.center());
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
        DEBUG << "Panned::drawForeground: visible rect " << pr << " differs from panned rect " << m_pannedRect << ", updating panned rect" <<endl;
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
Panned::mousePressEvent(QMouseEvent *ev)
{
    if (dragMode() != QGraphicsView::ScrollHandDrag ||
        ev->button() != Qt::LeftButton) {
        QGraphicsView::mousePressEvent(ev);
        return;
    }

    DEBUG << "Panned::mousePressEvent: have left button in drag mode" << endl;

    setDragMode(QGraphicsView::NoDrag);
    QGraphicsView::mousePressEvent(ev);
    setDragMode(QGraphicsView::ScrollHandDrag);

    if (!ev->isAccepted()) {
        ev->accept();
        m_dragging = true;
        m_lastDragPos = ev->pos();
        m_lastDragStart = ev->pos();
        m_lastOrigin = QPoint(horizontalScrollBar()->value(),
                              verticalScrollBar()->value());
        m_velocity = QPointF(0, 0);
        m_dragTimer->start(m_dragTimerMs);
        m_dragDirection = UnknownDrag;
    }
}

void
Panned::updateDragDirection(QPoint pos)
{
    if (m_dragDirection == FreeDrag) {
        return;
    }

    QPoint overall = pos - m_lastDragStart;

    int smallThreshold = 10;
    int largeThreshold = 30;
    int dx = qAbs(overall.x());
    int dy = qAbs(overall.y());

    switch (m_dragDirection) {

    case UnknownDrag:
        if (dx > smallThreshold) {
            if (dy > smallThreshold) {
                m_dragDirection = FreeDrag;
            } else {
                m_dragDirection = HorizontalDrag;
            }
        } else if (dy > smallThreshold) {
            m_dragDirection = VerticalDrag;
        }
        break;

    case HorizontalDrag:
        if (dy > largeThreshold) {
            m_dragDirection = FreeDrag;
        }
        break;

    case VerticalDrag:
        if (dx > largeThreshold) {
            m_dragDirection = FreeDrag;
        }
        break;
    };
}

void
Panned::mouseMoveEvent(QMouseEvent *ev)
{
    if (!m_dragging) {
        QGraphicsView::mouseMoveEvent(ev);
        return;
    }
    DEBUG << "Panned::mouseMoveEvent: dragging" << endl;
    ev->accept();
    updateDragDirection(ev->pos());
    QScrollBar *hBar = horizontalScrollBar();
    QScrollBar *vBar = verticalScrollBar();
    QPoint delta = ev->pos() - m_lastDragPos;
    if (m_dragDirection != VerticalDrag) {
        hBar->setValue(hBar->value() +
                       (isRightToLeft() ? delta.x() : -delta.x()));
    }
    if (m_dragDirection != HorizontalDrag) {
        vBar->setValue(vBar->value() - delta.y());
    }
    m_lastDragPos = ev->pos();
}

void
Panned::mouseReleaseEvent(QMouseEvent *ev)
{
    if (!m_dragging) {
        QGraphicsView::mouseReleaseEvent(ev);
        return;
    }
    DEBUG << "Panned::mouseReleaseEvent: dragging" << endl;
    ev->accept();
    m_dragging = false;
}

void
Panned::dragTimerTimeout()
{
    QPoint origin = QPoint(horizontalScrollBar()->value(),
                           verticalScrollBar()->value());
    if (m_dragging) {
        m_velocity = QPointF
            (float(origin.x() - m_lastOrigin.x()) / m_dragTimerMs,
             float(origin.y() - m_lastOrigin.y()) / m_dragTimerMs);
        m_lastOrigin = origin;
        DEBUG << "Panned::dragTimerTimeout: velocity = " << m_velocity << endl;
    } else {
        if (origin == m_lastOrigin) {
            m_dragTimer->stop();
        }
        float x = m_velocity.x(), y = m_velocity.y();
        if (fabsf(x) > 1.0/m_dragTimerMs) x = x * 0.9f;
        else x = 0.f;
        if (fabsf(y) > 1.0/m_dragTimerMs) y = y * 0.9f;
        else y = 0.f;
        m_velocity = QPointF(x, y);
        DEBUG << "Panned::dragTimerTimeout: velocity adjusted to " << m_velocity << endl;
        m_lastOrigin = origin;
        //!!! need to store origin in floats
        if (m_dragDirection != VerticalDrag) {
            horizontalScrollBar()->setValue(m_lastOrigin.x() +
                                            m_velocity.x() * m_dragTimerMs);
        }
        if (m_dragDirection != HorizontalDrag) {
            verticalScrollBar()->setValue(m_lastOrigin.y() +
                                          m_velocity.y() * m_dragTimerMs);
        }
    }
}

void
Panned::leaveEvent(QEvent *)
{
    emit mouseLeaves();
}
