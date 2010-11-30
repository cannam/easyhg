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
class ChangesetDetailItem;

class ChangesetItem : public QObject, public QGraphicsItem
{
    Q_OBJECT
    Q_INTERFACES(QGraphicsItem)

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

    bool isCurrent() const { return m_current; }
    void setCurrent(bool c) { m_current = c; }

    bool isNew() const { return m_new; }
    void setNew(bool n) { m_new = n; }

    bool shouldShowBranch() const { return m_showBranch; }
    void setShowBranch(bool s) { m_showBranch = s; }

signals:
    void detailShown();
    void detailHidden();

public slots:
    void showDetail();
    void hideDetail();

protected:
    virtual void mousePressEvent(QGraphicsSceneMouseEvent *);

private:
    void activateMenu();

    QFont m_font;
    Changeset *m_changeset;
    ChangesetDetailItem *m_detail;
    bool m_showBranch;
    int m_column;
    int m_row;
    bool m_wide;
    bool m_current;
    bool m_new;
};

#endif // CHANGESETITEM_H
