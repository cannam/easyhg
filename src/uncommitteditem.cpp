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

#include "uncommitteditem.h"
#include "colourset.h"
#include "debug.h"
#include "textabbrev.h"
#include "common.h"

#include <QPainter>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QMenu>
#include <QAction>
#include <QLabel>
#include <QWidgetAction>

UncommittedItem::UncommittedItem() :
    m_showBranch(false), m_isNewBranch(false), m_isMerge(false),
    m_column(0), m_row(0), m_wide(false)
{
    m_font = QFont();
    m_font.setPixelSize(scalePixelSize(11));
    m_font.setBold(false);
    m_font.setItalic(false);
    setCursor(Qt::ArrowCursor);
}

QRectF
UncommittedItem::boundingRect() const
{
    //!!! this stuff is gross, refactor with changesetitem and connectionitem
    int w = 100;
    if (m_wide) w = 180;
    return QRectF(-scalePixelSize((w-50)/2 - 1),
                  -scalePixelSize(30),
                  scalePixelSize(w - 3),
                  scalePixelSize(79 + 40));
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
UncommittedItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *e)
{
    DEBUG << "UncommittedItem::mouseDoubleClickEvent" << endl;
    if (e->button() == Qt::LeftButton) {
        emit showWork();
    }
}

void
UncommittedItem::activateMenu()
{
    QMenu *menu = new QMenu;
    QLabel *label = new QLabel
        (m_isMerge ?
         tr("<qt><b>&nbsp;Uncommitted merge</b></qt>") :
         tr("<qt><b>&nbsp;Uncommitted changes</b></qt>"));
    QWidgetAction *wa = new QWidgetAction(menu);
    wa->setDefaultWidget(label);
    menu->addAction(wa);
    menu->addSeparator();

    QAction *dif = menu->addAction(tr("Diff"));
    connect(dif, SIGNAL(triggered()), this, SIGNAL(diff()));
    QAction *stat = menu->addAction(tr("Summarise changes"));
    connect(stat, SIGNAL(triggered()), this, SIGNAL(showSummary()));
    
    menu->addSeparator();

    QAction *commit = menu->addAction(tr("Commit..."));
    connect(commit, SIGNAL(triggered()), this, SIGNAL(commit()));
    QAction *revert = menu->addAction(tr("Revert..."));
    connect(revert, SIGNAL(triggered()), this, SIGNAL(revert()));

    menu->addSeparator();

    QAction *branch = menu->addAction(tr("Start new branch..."));
    connect(branch, SIGNAL(triggered()), this, SIGNAL(newBranch()));
    QAction *nobranch = menu->addAction(tr("Cancel new branch"));
    nobranch->setEnabled(m_isNewBranch);
    connect(nobranch, SIGNAL(triggered()), this, SIGNAL(noBranch()));

    menu->exec(QCursor::pos());

    ungrabMouse();
}

void
UncommittedItem::paint(QPainter *paint, const QStyleOptionGraphicsItem *, QWidget *)
{
    if (isMerge()) {
        paintMerge(paint);
    } else {
        paintNormal(paint);
    }
}

void
UncommittedItem::paintNormal(QPainter *paint)
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
    int x0 = -scalePixelSize((width - 50) / 2 - 1);

    width = scalePixelSize(width);
    int half = scalePixelSize(50);
    int height = scalePixelSize(49);
    
    QRectF r(x0, 0, width - 3, height);
    paint->setBrush(Qt::white);
    paint->drawRoundedRect(r, 7, 7);

    if (m_wide) {
        QString label = tr("Uncommitted changes");
        paint->drawText(-(fm.width(label) - half)/2,
                        height/2 - fm.height()/2 + fm.ascent(),
                        label);
    } else {
        QString label = tr("Uncommitted");
        paint->drawText(-(fm.width(label) - half)/2,
                        height/2 - fm.height() + fm.ascent(),
                        label);
        label = tr("changes");
        paint->drawText(-(fm.width(label) - half)/2,
                        height/2 + fm.ascent(),
                        label);
    }        

    if (m_showBranch && m_branch != "") {
        // write branch name
        f.setBold(true);
        paint->setFont(f);
        int wid = width - 3;
        QString b = TextAbbrev::abbreviate(m_branch, QFontMetrics(f), wid);
        paint->drawText(x0, -fh + fm.ascent() - 4, b);
        f.setBold(false);
    }

    paint->restore();
    return;
}

void
UncommittedItem::paintMerge(QPainter *paint)
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

    int size = fh * 2;
    int x0 = -size/2 + scalePixelSize(25);

    paint->setBrush(Qt::white);
    paint->drawEllipse(QRectF(x0, fh, size, size));
    
    if (m_wide) {
        QString label = tr("Uncommitted merge");
        paint->drawText(size/2 + scalePixelSize(28),
                        scalePixelSize(25) - fm.height()/2 + fm.ascent(),
                        label);
    } else {
        QString label = tr("Uncommitted");
        paint->drawText(size/2 + scalePixelSize(28),
                        scalePixelSize(25) - fm.height() + fm.ascent(),
                        label);
        label = tr("merge");
        paint->drawText(size/2 + scalePixelSize(28),
                        scalePixelSize(25) + fm.ascent(),
                        label);
    }        

    if (m_showBranch && m_branch != "") {
        // write branch name
        f.setBold(true);
        paint->setFont(f);
	int wid = size * 3;
	QString branch = TextAbbrev::abbreviate(m_branch, QFontMetrics(f), wid);
	paint->drawText(-wid/2 + scalePixelSize(25), fm.ascent() - 4, branch);
    }

    paint->restore();
    return;
}
