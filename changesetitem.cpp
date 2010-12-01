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

#include "changesetitem.h"
#include "changesetdetailitem.h"
#include "changeset.h"
#include "textabbrev.h"
#include "colourset.h"
#include "debug.h"

#include <QPainter>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QMenu>
#include <QAction>
#include <QLabel>
#include <QWidgetAction>

ChangesetItem::ChangesetItem(Changeset *cs) :
    m_changeset(cs), m_detail(0),
    m_showBranch(false), m_column(0), m_row(0), m_wide(false),
    m_current(false), m_new(false)
{
    m_font = QFont();
    m_font.setPixelSize(11);
    m_font.setBold(false);
    m_font.setItalic(false);
}

QString
ChangesetItem::getId()
{
    return m_changeset->id();
}

QRectF
ChangesetItem::boundingRect() const
{
    int w = 100;
    if (m_wide) w = 180;
    return QRectF(-((w-50)/2 - 1), -30, w - 3, 79);
}

void
ChangesetItem::showDetail()
{
    if (m_detail) return;
    m_detail = new ChangesetDetailItem(m_changeset);
    m_detail->setZValue(zValue() + 1);
    scene()->addItem(m_detail);
    int w = 100;
    if (m_wide) w = 180;
    int h = 80;
//    m_detail->moveBy(x() - (m_detail->boundingRect().width() - 50) / 2,
//                     y() + 60);
    m_detail->moveBy(x() + (w + 50) / 2 + 10 + 0.5,
                     y() - (m_detail->boundingRect().height() - h) / 2 + 0.5);
    emit detailShown();
}    

void
ChangesetItem::hideDetail()
{
    if (!m_detail) return;
    scene()->removeItem(m_detail);
    delete m_detail;
    m_detail = 0;
    emit detailHidden();
}    

void
ChangesetItem::mousePressEvent(QGraphicsSceneMouseEvent *e)
{
    DEBUG << "ChangesetItem::mousePressEvent" << endl;
    if (e->button() == Qt::LeftButton) {
        if (m_detail) {
            hideDetail();
        } else {
            showDetail();
        }
    } else if (e->button() == Qt::RightButton) {
        if (m_detail) {
            hideDetail();
        }
        activateMenu();
    }
}

void
ChangesetItem::activateMenu()
{
    QMenu *menu = new QMenu;
    QLabel *label = new QLabel(tr("<qt><b>Revision: </b>%1</qt>")
                               .arg(m_changeset->id()));
    QWidgetAction *wa = new QWidgetAction(menu);
    wa->setDefaultWidget(label);
    menu->addAction(wa);
    menu->addSeparator();

    QAction *update = menu->addAction(tr("Update to this revision"));
    connect(update, SIGNAL(triggered()), this, SLOT(updateActivated()));

    menu->addSeparator();

    QAction *diffParent = menu->addAction(tr("Diff against previous revision"));
    connect(diffParent, SIGNAL(triggered()), this, SLOT(diffToPreviousActivated()));
    QAction *diffCurrent = menu->addAction(tr("Diff against current revision"));
    connect(diffCurrent, SIGNAL(triggered()), this, SLOT(diffToCurrentActivated()));

    menu->addSeparator();

    QAction *merge = menu->addAction(tr("Merge from here to current"));
    connect(merge, SIGNAL(triggered()), this, SLOT(mergeActivated()));
    QAction *tag = menu->addAction(tr("Tag this revision"));
    connect(tag, SIGNAL(triggered()), this, SLOT(tagActivated()));

    menu->exec(QCursor::pos());

    ungrabMouse();
}

void ChangesetItem::updateActivated() { emit updateTo(getId()); }
void ChangesetItem::diffToPreviousActivated() { emit diffToParent(getId(), m_changeset->parents()[0]); } //!!! no, this is most likely to be useful when something has more than one parent!
void ChangesetItem::diffToCurrentActivated() { emit diffToCurrent(getId()); }
void ChangesetItem::mergeActivated() { emit mergeFrom(getId()); }
void ChangesetItem::tagActivated() { emit tag(getId()); }

void
ChangesetItem::paint(QPainter *paint, const QStyleOptionGraphicsItem *option,
                     QWidget *w)
{
    paint->save();
    
    ColourSet *colourSet = ColourSet::instance();
    QColor branchColour = colourSet->getColourFor(m_changeset->branch());
    QColor userColour = colourSet->getColourFor(m_changeset->author());

    QFont f(m_font);

    QTransform t = paint->worldTransform();
    float scale = std::min(t.m11(), t.m22());
    if (scale > 1.0) {
	int ps = int((f.pixelSize() / scale) + 0.5);
	if (ps < 8) ps = 8;
	f.setPixelSize(ps);
    }

    if (scale < 0.1) {
	paint->setPen(QPen(branchColour, 0));
    } else {
	paint->setPen(QPen(branchColour, 2));
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

    if (m_new) {
        paint->save();
        paint->setPen(Qt::yellow);
        paint->drawRect(QRectF(x0 - 2, -2, width + 1, height + 4));
        paint->restore();
    }

    if (m_current) {
        paint->drawRect(QRectF(x0 - 4, -4, width + 5, height + 8));
    }

    if (scale < 0.2) {
	paint->restore();
	return;
    }

    paint->fillRect(QRectF(x0 + 0.5, 0.5, width - 4, fh - 0.5),
		    QBrush(userColour));

    paint->setPen(QPen(Qt::white));

    int wid = width - 5;
    QString person = TextAbbrev::abbreviate(m_changeset->authorName(), fm, wid);
    paint->drawText(x0 + 3, fm.ascent(), person);

    paint->setPen(QPen(Qt::black));

    QStringList tags = m_changeset->tags();
    if (!tags.empty()) {
        QStringList nonTipTags;
        foreach (QString t, tags) {
            // I'm not convinced that showing the tip tag really
            // works; I think perhaps it confuses as much as it
            // illuminates.  But also, our current implementation
            // doesn't interact well with it because it moves -- it's
            // the one thing that can actually (in normal use) change
            // inside an existing changeset record even during an
            // incremental update
            if (t != "tip") nonTipTags.push_back(t);
        }
        if (!nonTipTags.empty()) {
            QString tagText = nonTipTags.join(" ").trimmed();
            int tw = fm.width(tagText);
            paint->fillRect(QRectF(x0 + width - 8 - tw, 1, tw + 4, fh - 1),
                            QBrush(Qt::yellow));
            paint->drawText(x0 + width - 6 - tw, fm.ascent(), tagText);
        }
    }

    if (m_showBranch) {
	// write branch name
	f.setBold(true);
	paint->setFont(f);
	QString branch = m_changeset->branch();
        if (branch == "") branch = "default";
	int wid = width - 3;
	branch = TextAbbrev::abbreviate(branch, QFontMetrics(f), wid);
	paint->drawText(x0, -fh + fm.ascent() - 4, branch);
	f.setBold(false);
    }

//    f.setItalic(true);
    fm = QFontMetrics(f);
    fh = fm.height();
    paint->setFont(f);

    QString comment = m_changeset->comment().trimmed();
    comment = comment.replace("\\n", " ");
    comment = comment.replace(QRegExp("^\"\\s*\\**\\s*"), "");
    comment = comment.replace(QRegExp("\"$"), "");
    comment = comment.replace("\\\"", "\"");

    wid = width - 5;
    int nlines = (height / fh) - 1;
    if (nlines < 1) nlines = 1;
    comment = TextAbbrev::abbreviate(comment, fm, wid, TextAbbrev::ElideEnd,
				     "...", nlines);

    QStringList lines = comment.split('\n');
    for (int i = 0; i < lines.size(); ++i) {
	paint->drawText(x0 + 3, i * fh + fh + fm.ascent(), lines[i].trimmed());
    }

    paint->restore();
}
