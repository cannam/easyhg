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

#include "changesetdetailitem.h"
#include "changeset.h"
#include "textabbrev.h"
#include "colourset.h"
#include "debug.h"
#include "common.h"

#include <QTextDocument>
#include <QPainter>
#include <QAbstractTextDocumentLayout>

ChangesetDetailItem::ChangesetDetailItem(Changeset *cs) :
    m_changeset(cs), m_doc(0)
{
    m_font = QFont();
    m_font.setPixelSize(11);
    m_font.setBold(false);
    m_font.setItalic(false);

    makeDocument();
}

ChangesetDetailItem::~ChangesetDetailItem()
{
    delete m_doc;
}

QRectF
ChangesetDetailItem::boundingRect() const
{
    int w = 350;
    m_doc->setTextWidth(w);
    return QRectF(-10, -10, w + 10, m_doc->size().height() + 10);
}

QVariant
ChangesetDetailItem::itemChange(GraphicsItemChange c, const QVariant &v)
{
    if (c == ItemVisibleHasChanged) {
        bool visible = v.toBool();
        DEBUG << "ChangesetDetailItem::itemChange: visible = " << visible << endl;
        if (visible && scene()) {
            ensureVisible();
        }
    }
    return v;
}

void
ChangesetDetailItem::paint(QPainter *paint,
			   const QStyleOptionGraphicsItem *option,
			   QWidget *w)
{
    paint->save();
    
    ColourSet *colourSet = ColourSet::instance();
    QColor branchColour = colourSet->getColourFor(m_changeset->branch());
    QColor userColour = colourSet->getColourFor(m_changeset->author());

    QTransform t = paint->worldTransform();
    float scale = std::min(t.m11(), t.m22());

    if (scale < 0.1) {
	paint->setPen(QPen(branchColour, 0));
    } else {
	paint->setPen(QPen(branchColour, 2));
    }
    
    int width = 350;
    m_doc->setTextWidth(width);
    int height = m_doc->size().height();

    QRectF r(0.5, 0.5, width - 1, height - 1);
    paint->setBrush(Qt::white);
    paint->drawRoundedRect(r, 10, 10);

    if (scale < 0.1) {
	paint->restore();
	return;
    }

    // little triangle connecting to its "owning" changeset item
    paint->setBrush(branchColour);
    QVector<QPointF> pts;
    pts.push_back(QPointF(0, height/3 - 5));
    pts.push_back(QPointF(0, height/3 + 5));
    pts.push_back(QPointF(-10, height/3));
    pts.push_back(QPointF(0, height/3 - 5));
    paint->drawPolygon(QPolygonF(pts));

/*
    paint->setBrush(branchColour);
    QVector<QPointF> pts;
    pts.push_back(QPointF(width/2 - 5, 0));
    pts.push_back(QPointF(width/2 + 5, 0));
    pts.push_back(QPointF(width/2, -10));
    pts.push_back(QPointF(width/2 - 5, 0));
    paint->drawPolygon(QPolygonF(pts));
*/
    m_doc->drawContents(paint, r);

    paint->restore();
}

void
ChangesetDetailItem::makeDocument()
{
    delete m_doc;
    m_doc = new QTextDocument;
    m_doc->setHtml(m_changeset->formatHtml());
    m_doc->setDefaultFont(m_font);
}

