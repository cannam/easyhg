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

#ifndef DATEITEM_H
#define DATEITEM_H

#include <QGraphicsObject>

class DateItem : public QGraphicsObject
{
    Q_OBJECT

public:
    DateItem();

    virtual QRectF boundingRect() const;
    virtual void paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget *);

    void setRows(int minrow, int n);
    void setCols(int mincol, int n);

    void setEven(bool e) { m_even = e; }

    QString dateString() const { return m_dateString; }
    void setDateString(QString s) { m_dateString = s; }

signals:
    void clicked();

protected:
    virtual void mousePressEvent(QGraphicsSceneMouseEvent *);

private:
    QString m_dateString;
    int m_minrow;
    int m_maxrow;
    int m_mincol;
    int m_maxcol;
    bool m_even;
};

#endif // DATEITEM_H