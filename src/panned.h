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

#ifndef _PANNED_H_
#define _PANNED_H_

#include <QGraphicsView>

class QWheelEvent;
class QEvent;
class QTimer;

class Panned : public QGraphicsView
{
    Q_OBJECT

public:
    Panned();
    virtual ~Panned() { }

    virtual void setScene(QGraphicsScene *s);

signals:
    void pannedRectChanged(QRectF);
    void wheelEventReceived(QWheelEvent *);
    void pannedContentsScrolled();
    void mouseLeaves();

public slots:
    void slotSetPannedRect(QRectF);
    void slotEmulateWheelEvent(QWheelEvent *ev);

    void zoomIn();
    void zoomOut();

private slots:
    void dragTimerTimeout();

protected:
    QRectF m_pannedRect;

    QPoint m_lastDragPos;
    QPoint m_lastOrigin;
    QPointF m_velocity;
    bool m_dragging;
    int m_dragTimerMs;
    QTimer *m_dragTimer;

    virtual void mousePressEvent(QMouseEvent *);
    virtual void mouseMoveEvent(QMouseEvent *);
    virtual void mouseReleaseEvent(QMouseEvent *);

    virtual void paintEvent(QPaintEvent *);
    virtual void resizeEvent(QResizeEvent *);
    virtual void drawForeground(QPainter *, const QRectF &);
    virtual void wheelEvent(QWheelEvent *);
    virtual void leaveEvent(QEvent *);
};

#endif

