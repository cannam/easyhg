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

#ifndef CHANGESETITEM_H
#define CHANGESETITEM_H

#include <QGraphicsItem>
#include <QFont>

class Changeset;

class ChangesetItem : public QGraphicsItem
{
public:
    ChangesetItem(Changeset *cs);

    virtual QRectF boundingRect() const;
    virtual void paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget *);

    Changeset *getChangeset() { return m_changeset; }

    int column() const { return m_column; }
    int row() const { return m_row; }
    void setColumn(int c) { m_column = c; setX(c * 100); }
    void setRow(int r) { m_row = r; setY(r * 90); }

    bool isWide() const { return m_wide; }
    void setWide(bool w) { m_wide = w; }

private:
    QFont m_font;
    Changeset *m_changeset;
    int m_column;
    int m_row;
    bool m_wide;
};

#endif // CHANGESETITEM_H
