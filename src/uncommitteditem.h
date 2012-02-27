/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    EasyMercurial

    Based on HgExplorer by Jari Korhonen
    Copyright (c) 2010 Jari Korhonen
    Copyright (c) 2012 Chris Cannam
    Copyright (c) 2012 Queen Mary, University of London
    
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef UNCOMMITTEDITEM_H
#define UNCOMMITTEDITEM_H

#include <QGraphicsObject>
#include <QFont>

class UncommittedItem : public QGraphicsObject
{
    Q_OBJECT

public:
    UncommittedItem();

    virtual QRectF boundingRect() const;
    virtual void paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget *);

    QString branch() const { return m_branch; }
    void setBranch(QString b) { m_branch = b; }

    bool showBranch() const { return m_showBranch; }
    void setShowBranch(bool s) { m_showBranch = s; }

    bool isNewBranch() const { return m_isNewBranch; }
    void setIsNewBranch(bool s) { m_isNewBranch = s; }

    bool isMerge() const { return m_isMerge; }
    void setIsMerge(bool m) { m_isMerge = m; }
    
    int column() const { return m_column; }
    int row() const { return m_row; }
    void setColumn(int c) { m_column = c; setX(c * 100); }
    void setRow(int r) { m_row = r; setY(r * 90); }

    bool isWide() const { return m_wide; }
    void setWide(bool w) { m_wide = w; }

signals:
    void commit();
    void revert();
    void diff();
    void showSummary();
    void showWork();
    void newBranch();
    void noBranch();

protected:
    virtual void mousePressEvent(QGraphicsSceneMouseEvent *);
    virtual void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *);

private:
    void activateMenu();

    QString m_branch;
    bool m_showBranch;
    bool m_isNewBranch;
    bool m_isMerge;
    QFont m_font;
    int m_column;
    int m_row;
    bool m_wide;

    void paintNormal(QPainter *);
    void paintMerge(QPainter *);
};

#endif // UNCOMMITTEDITEM_H
