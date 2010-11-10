/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2010 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "panner.h"
#include "panned.h"

#include <QPolygon>
#include <QMouseEvent>
#include <QColor>
#include <QGLWidget>

#include <iostream>

class PannerScene : public QGraphicsScene
{
public:
    friend class Panner;
};

Panner::Panner() :
    m_clicked(false)
{
//    setViewport(new QGLWidget());
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setOptimizationFlags(QGraphicsView::DontSavePainterState);
    setMouseTracking(true);
    setInteractive(false);
}

void
Panner::setScene(QGraphicsScene *s)
{
    if (scene()) {
        disconnect(scene(), SIGNAL(sceneRectChanged(const QRectF &)),
                   this, SLOT(slotSceneRectChanged(const QRectF &)));
    }
    QGraphicsView::setScene(s);
    if (scene()) fitInView(sceneRect(), Qt::IgnoreAspectRatio);
    m_cache = QPixmap();
    connect(scene(), SIGNAL(sceneRectChanged(const QRectF &)),
            this, SLOT(slotSceneRectChanged(const QRectF &)));
}

void
Panner::connectToPanned(Panned *p)
{
    connect(p, SIGNAL(pannedRectChanged(QRectF)),
            this, SLOT(slotSetPannedRect(QRectF)));

    connect(this, SIGNAL(pannedRectChanged(QRectF)),
            p, SLOT(slotSetPannedRect(QRectF)));
}

void
Panner::slotSetPannedRect(QRectF rect) 
{
    m_pannedRect = rect;
    viewport()->update();
}

void
Panner::resizeEvent(QResizeEvent *)
{
    if (scene()) fitInView(sceneRect(), Qt::IgnoreAspectRatio);
    m_cache = QPixmap();
}

void
Panner::slotSceneRectChanged(const QRectF &newRect)
{
    if (!scene()) return; // spurious
    fitInView(newRect, Qt::IgnoreAspectRatio);
    m_cache = QPixmap();
    viewport()->update();
}

void
Panner::paintEvent(QPaintEvent *e)
{
    QPaintEvent *e2 = new QPaintEvent(e->region().boundingRect());
    QGraphicsView::paintEvent(e2);

    QPainter paint;
    paint.begin(viewport());
    paint.setClipRegion(e->region());

    QPainterPath path;
    path.addRect(rect());
    path.addPolygon(mapFromScene(m_pannedRect));

    QColor c(QColor::fromHsv(211, 194, 238));
    c.setAlpha(80);
    paint.setPen(Qt::NoPen);
    paint.setBrush(c);
    paint.drawPath(path);

    paint.setBrush(Qt::NoBrush);
    paint.setPen(QPen(QColor::fromHsv(211, 194, 238), 0));
    paint.drawConvexPolygon(mapFromScene(m_pannedRect));

    paint.end();

    emit pannerChanged(m_pannedRect);
}

void
Panner::updateScene(const QList<QRectF> &rects)
{
    if (!m_cache.isNull()) m_cache = QPixmap();
    QGraphicsView::updateScene(rects);
}

void
Panner::drawItems(QPainter *painter, int numItems,
                  QGraphicsItem *items[],
                  const QStyleOptionGraphicsItem options[])
{
    if (m_cache.size() != viewport()->size()) {

        QGraphicsScene *s = scene();
        if (!s) return;
        PannerScene *ps = static_cast<PannerScene *>(s);

        m_cache = QPixmap(viewport()->size());
        m_cache.fill(Qt::transparent);
        QPainter cachePainter;
        cachePainter.begin(&m_cache);
        cachePainter.setTransform(viewportTransform());
        ps->drawItems(&cachePainter, numItems, items, options);
        cachePainter.end();
    }

    painter->save();
    painter->setTransform(QTransform());
    painter->drawPixmap(0, 0, m_cache);
    painter->restore();
}
 
void
Panner::mousePressEvent(QMouseEvent *e)
{
    if (e->button() != Qt::LeftButton) {
        QGraphicsView::mouseDoubleClickEvent(e);
        return;
    }
    m_clicked = true;
    m_clickedRect = m_pannedRect;
    m_clickedPoint = e->pos();
}

void
Panner::mouseDoubleClickEvent(QMouseEvent *e)
{
    if (e->button() != Qt::LeftButton) {
        QGraphicsView::mouseDoubleClickEvent(e);
        return;
    }

    moveTo(e->pos());
}

void
Panner::mouseMoveEvent(QMouseEvent *e)
{
    if (!m_clicked) return;
    QPointF cp = mapToScene(m_clickedPoint);
    QPointF mp = mapToScene(e->pos());
    QPointF delta = mp - cp;
    QRectF nr = m_clickedRect;
    nr.translate(delta);
    slotSetPannedRect(nr);
    emit pannedRectChanged(m_pannedRect);
    viewport()->update();
}

void
Panner::mouseReleaseEvent(QMouseEvent *e)
{
    if (e->button() != Qt::LeftButton) {
        QGraphicsView::mouseDoubleClickEvent(e);
        return;
    }

    if (m_clicked) {
        mouseMoveEvent(e);
    }

    m_clicked = false;
    viewport()->update();
}

void
Panner::wheelEvent(QWheelEvent *e)
{
    if (e->delta() > 0) {
        emit zoomOut();
    } else {
        emit zoomIn();
    }
}

void
Panner::moveTo(QPoint p)
{
    QPointF sp = mapToScene(p);
    QRectF nr = m_pannedRect;
    double d = sp.x() - nr.center().x();
    nr.translate(d, 0);
    slotSetPannedRect(nr);
    emit pannedRectChanged(m_pannedRect);
    viewport()->update();
}
   
