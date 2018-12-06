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

#include "changesetview.h"
#include "changesetscene.h"
#include "colourset.h"
#include "debug.h"
#include "common.h"

#include <QScrollBar>

ChangesetView::ChangesetView() :
    Panned()
{
    connect(horizontalScrollBar(), SIGNAL(valueChanged(int)),
	    this, SLOT(horizontalScrollHappened()));
}

void
ChangesetView::horizontalScrollHappened()
{
    DEBUG << "ChangesetView::horizontalScrollHappened" << endl;
    invalidateScene(rect(), QGraphicsScene::BackgroundLayer);
    viewport()->update();
}

void
ChangesetView::drawBackground(QPainter *paint, const QRectF &rect)
{
    DEBUG << "ChangesetView::drawBackground" << endl;

    ChangesetScene *cs = qobject_cast<ChangesetScene *>(scene());

    if (!cs) {
	QGraphicsView::drawBackground(paint, rect);
	return;
    }

    DEBUG << "ChangesetView::drawBackground: have scene" << endl;

    ChangesetScene::DateRanges ranges = cs->getDateRanges();

    paint->setClipRect(rect);

    DEBUG << "clip rect is " << rect << endl;

    paint->save();
    QFont f(paint->font());
    f.setBold(true);
    paint->setFont(f);

    float x = mapToScene(0, 0).x();
    float w = mapToScene(width(), 0).x() - x;
    float px = mapToScene(5, 0).x();

    QBrush oddBrush(QColor::fromRgb(250, 250, 250));
    QBrush evenBrush(QColor::fromRgb(240, 240, 240));

    //!!! todo: select only the ranges actually within range!
    
    for (ChangesetScene::DateRanges::const_iterator i = ranges.begin();
         i != ranges.end(); ++i) {

	ChangesetScene::DateRange range = i.value();

        int rowSpace = scalePixelSize(90);
        
        QRectF r = QRectF(x, range.minrow * rowSpace - scalePixelSize(25),
			  w, range.nrows * rowSpace).normalized();

	paint->fillRect(r, range.even ? evenBrush : oddBrush);
        paint->drawText(px, range.minrow * rowSpace - scalePixelSize(10), range.label);
    }

    paint->restore();
}


