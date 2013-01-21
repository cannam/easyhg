/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    EasyMercurial

    Based on HgExplorer by Jari Korhonen
    Copyright (c) 2010 Jari Korhonen
    Copyright (c) 2013 Chris Cannam
    Copyright (c) 2013 Queen Mary, University of London
    
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef CHANGESETDETAILITEM_H
#define CHANGESETDETAILITEM_H

#include <QGraphicsItem>
#include <QFont>

class Changeset;

class ChangesetDetailItem : public QGraphicsItem
{
public:
    ChangesetDetailItem(Changeset *cs);
    virtual ~ChangesetDetailItem();

    virtual QRectF boundingRect() const;
    virtual void paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget *);

    Changeset *getChangeset() { return m_changeset; }

private:
    QFont m_font;
    Changeset *m_changeset;
    QTextDocument *m_doc;

    QVariant itemChange(GraphicsItemChange, const QVariant &);

    void makeDocument();
};

#endif // CHANGESETDETAILITEM_H
