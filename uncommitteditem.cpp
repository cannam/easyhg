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

#include "uncommitteditem.h"
#include "colourset.h"
#include "debug.h"

#include <QPainter>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QMenu>
#include <QAction>
#include <QLabel>
#include <QWidgetAction>

UncommittedItem::UncommittedItem() :
    m_column(0), m_row(0), m_wide(false)
{
    m_font = QFont();
    m_font.setPixelSize(11);
    m_font.setBold(false);
    m_font.setItalic(false);
}

QRectF
UncommittedItem::boundingRect() const
{
    //!!! this stuff is gross, refactor with changesetitem and connectionitem
    int w = 100;
    if (m_wide) w = 180;
    return QRectF(-((w-50)/2 - 1), -30, w - 3, 79 + 40);
}

void
UncommittedItem::mousePressEvent(QGraphicsSceneMouseEvent *e)
{
    DEBUG << "UncommittedItem::mousePressEvent" << endl;
    if (e->button() == Qt::RightButton) {
        activateMenu();
    }
}

void
UncommittedItem::activateMenu()
{
    QMenu *menu = new QMenu;
    QLabel *label = new QLabel(tr("<qt><b>Uncommitted changes</b></qt>"));
    QWidgetAction *wa = new QWidgetAction(menu);
    wa->setDefaultWidget(label);
    menu->addAction(wa);
    menu->addSeparator();

    QAction *commit = menu->addAction(tr("Commit..."));
    connect(commit, SIGNAL(triggered()), this, SIGNAL(commit()));
    QAction *revert = menu->addAction(tr("Revert..."));
    connect(revert, SIGNAL(triggered()), this, SIGNAL(revert()));
    QAction *dif = menu->addAction(tr("Diff"));
    connect(dif, SIGNAL(triggered()), this, SIGNAL(diff()));

    menu->exec(QCursor::pos());

    ungrabMouse();
}

void
UncommittedItem::paint(QPainter *paint, const QStyleOptionGraphicsItem *option,
		       QWidget *w)
{
    paint->save();
    
    ColourSet *colourSet = ColourSet::instance();
    QColor branchColour = colourSet->getColourFor(m_branch);

    QFont f(m_font);

    QTransform t = paint->worldTransform();
    float scale = std::min(t.m11(), t.m22());
    if (scale > 1.0) {
	int ps = int((f.pixelSize() / scale) + 0.5);
	if (ps < 8) ps = 8;
	f.setPixelSize(ps);
    }

    if (scale < 0.1) {
	paint->setPen(QPen(branchColour, 0, Qt::DashLine));
    } else {
	paint->setPen(QPen(branchColour, 2, Qt::DashLine));
    }
	
    paint->setFont(f);
    QFontMetrics fm(f);
    int fh = fm.height();

    int width = 100;
    if (m_wide) width = 180;
    int x0 = -((width - 50) / 2 - 1);

    int height = 49;
    QRectF r(x0, 0, width - 3, height);
    paint->drawRect(r);

    paint->drawLine(x0 + width/2, height, x0 + width/2, height + 40);

    QString label = tr("Uncommitted changes");
    paint->drawText(-(fm.width(label) - 50)/2, 25 - fm.height()/2 + fm.ascent(), label);

    paint->restore();
    return;
}
