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

#include "changesetitem.h"
#include "changesetscene.h"
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
#include <QApplication>
#include <QClipboard>

QImage *ChangesetItem::m_star = 0;

ChangesetItem::ChangesetItem(Changeset *cs) :
    m_changeset(cs), m_detail(0), m_detailVisible(false),
    m_showBranch(false), m_column(0), m_row(0), m_wide(false),
    m_current(false), m_closing(false), m_new(false), m_searchMatches(false)
{
    m_font = QFont();
    m_font.setPixelSize(11);
    m_font.setBold(false);
    m_font.setItalic(false);
    setCursor(Qt::ArrowCursor);

    if (!m_star) m_star = new QImage(":images/star.png");
}

ChangesetItem::~ChangesetItem()
{
    if (m_detail && !m_detailVisible) delete m_detail;
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
    return QRectF(-((w-50)/2 - 1), -30, w - 3, 90);
}

void
ChangesetItem::showDetail()
{
    if (m_detailVisible) return;
    if (!m_detail) {
        m_detail = new ChangesetDetailItem(m_changeset);
        m_detail->setZValue(zValue() + 1);
    }
    scene()->addItem(m_detail);
    int w = 100;
    if (m_wide) w = 180;
    if (isMerge() || isClosingCommit()) w = 60;
    int h = 80;
    m_detail->setPos(x() + (w + 50) / 2 + 10 + 0.5,
                     y() - (m_detail->boundingRect().height() - h) / 3 + 0.5);
    m_detailVisible = true;
    emit detailShown();
}    

void
ChangesetItem::hideDetail()
{
    if (!m_detailVisible) return;
    scene()->removeItem(m_detail);
    m_detailVisible = false;
    emit detailHidden();
}    

bool
ChangesetItem::matchSearchText(QString text)
{
    m_searchText = text;
    m_searchMatches = false;
    if (m_showBranch) {
        m_searchMatches = (m_changeset->branch().contains
                           (text, Qt::CaseInsensitive));
    }
    if (!m_searchMatches) {
        m_searchMatches = (m_changeset->comment().contains
                           (text, Qt::CaseInsensitive));
    }
    return m_searchMatches;
}

void
ChangesetItem::mousePressEvent(QGraphicsSceneMouseEvent *e)
{
    DEBUG << "ChangesetItem::mousePressEvent" << endl;
    if (e->button() == Qt::LeftButton) {
        if (m_detailVisible) {
            hideDetail();
        } else {
            showDetail();
        }
    }
}

void
ChangesetItem::contextMenuEvent(QGraphicsSceneContextMenuEvent *)
{
    if (m_detail) {
        hideDetail();
    }

    m_parentDiffActions.clear();
    m_summaryActions.clear();

    QMenu *menu = new QMenu;
    QLabel *label = new QLabel(tr("<qt><b>&nbsp;Revision: </b>%1</qt>")
                               .arg(Changeset::hashOf(m_changeset->id())));
    QWidgetAction *wa = new QWidgetAction(menu);
    wa->setDefaultWidget(label);
    menu->addAction(wa);
    menu->addSeparator();

    QAction *copyId = menu->addAction(tr("Copy identifier to clipboard"));
    connect(copyId, SIGNAL(triggered()), this, SLOT(copyIdActivated()));

    QAction *stat = menu->addAction(tr("Summarise changes"));
    connect(stat, SIGNAL(triggered()), this, SLOT(showSummaryActivated()));

    menu->addSeparator();

    QStringList parents = m_changeset->parents();

    QString leftId, rightId;
    bool havePositions = false;

    if (parents.size() > 1) {
        ChangesetScene *cs = dynamic_cast<ChangesetScene *>(scene());
        if (cs && parents.size() == 2) {
            ChangesetItem *i0 = cs->getItemById(parents[0]);
            ChangesetItem *i1 = cs->getItemById(parents[1]);
            if (i0 && i1) {
                if (i0->x() < i1->x()) {
                    leftId = parents[0];
                    rightId = parents[1];
                } else {
                    leftId = parents[1];
                    rightId = parents[0];
                }
                havePositions = true;
            }
        }
    }

    if (parents.size() > 1) {
        if (havePositions) {
            
            QAction *diff = menu->addAction(tr("Diff to left parent"));
            connect(diff, SIGNAL(triggered()), this, SLOT(diffToParentActivated()));
            m_parentDiffActions[diff] = leftId;
            
            diff = menu->addAction(tr("Diff to right parent"));
            connect(diff, SIGNAL(triggered()), this, SLOT(diffToParentActivated()));
            m_parentDiffActions[diff] = rightId;

        } else {

            foreach (QString parentId, parents) {
                QString text = tr("Diff to parent %1").arg(Changeset::hashOf(parentId));
                QAction *diff = menu->addAction(text);
                connect(diff, SIGNAL(triggered()), this, SLOT(diffToParentActivated()));
                m_parentDiffActions[diff] = parentId;
            }
        }

    } else {

        QAction *diff = menu->addAction(tr("Diff to parent"));
        connect(diff, SIGNAL(triggered()), this, SLOT(diffToParentActivated()));
    }

    QAction *diffCurrent = menu->addAction(tr("Diff to current working folder"));
    connect(diffCurrent, SIGNAL(triggered()), this, SLOT(diffToCurrentActivated()));

    menu->addSeparator();

    QAction *update = menu->addAction(tr("Update to this revision"));
    connect(update, SIGNAL(triggered()), this, SLOT(updateActivated()));

    QAction *merge = menu->addAction(tr("Merge from here to current"));
    connect(merge, SIGNAL(triggered()), this, SLOT(mergeActivated()));

    menu->addSeparator();

    QAction *branch = menu->addAction(tr("Start new branch..."));
    branch->setEnabled(m_current);
    connect(branch, SIGNAL(triggered()), this, SLOT(newBranchActivated()));

    QAction *closebranch = menu->addAction(tr("Close branch..."));
    closebranch->setEnabled(m_current);
    connect(closebranch, SIGNAL(triggered()), this, SLOT(closeBranchActivated()));

    QAction *tag = menu->addAction(tr("Add tag..."));
    connect(tag, SIGNAL(triggered()), this, SLOT(tagActivated()));

    ungrabMouse();

    menu->exec(QCursor::pos());
}

void
ChangesetItem::copyIdActivated()
{
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(Changeset::hashOf(m_changeset->id()));
}

void ChangesetItem::diffToParentActivated()
{
    QAction *a = qobject_cast<QAction *>(sender());
    QString parentId;
    if (m_parentDiffActions.contains(a)) {
        parentId = m_parentDiffActions[a];
        DEBUG << "ChangesetItem::diffToParentActivated: specific parent " 
              << parentId << " selected" << endl;
    } else {
        parentId = m_changeset->parents()[0];
        DEBUG << "ChangesetItem::diffToParentActivated: "
              << "no specific parent selected, using first parent "
              << parentId << endl;
    }

    emit diffToParent(getId(), parentId);
}

void ChangesetItem::showSummaryActivated()
{
    emit showSummary(m_changeset);
}

void ChangesetItem::updateActivated() { emit updateTo(getId()); }
void ChangesetItem::diffToCurrentActivated() { emit diffToCurrent(getId()); }
void ChangesetItem::mergeActivated() { emit mergeFrom(getId()); }
void ChangesetItem::tagActivated() { emit tag(getId()); }
void ChangesetItem::newBranchActivated() { emit newBranch(getId()); }
void ChangesetItem::closeBranchActivated() { emit closeBranch(getId()); }

void
ChangesetItem::paint(QPainter *paint, const QStyleOptionGraphicsItem *, QWidget *)
{
    if (isClosingCommit() || isMerge()) {
        paintSimple(paint);
    } else {
        paintNormal(paint);
    }
}

bool
ChangesetItem::isMerge() const
{
    return (m_changeset && m_changeset->parents().size() > 1);
}

bool
ChangesetItem::isClosed() const
{
    return (m_changeset && m_changeset->closed());
}

void
ChangesetItem::paintNormal(QPainter *paint)
{
    paint->save();
    
    int alpha = 255;
    if (isClosed()) alpha = 90;

    ColourSet *colourSet = ColourSet::instance();
    QColor branchColour = colourSet->getColourFor(m_changeset->branch());
    QColor userColour = colourSet->getColourFor(m_changeset->author());

    branchColour.setAlpha(alpha);
    userColour.setAlpha(alpha);

    QFont f(m_font);

    QTransform t = paint->worldTransform();
    float scale = std::min(t.m11(), t.m22());

    if (scale > 1.0) {
#ifdef Q_OS_WIN32
        f.setHintingPreference(QFont::PreferVerticalHinting);
#endif
    }

    bool showText = (scale >= 0.2);
    bool showProperLines = (scale >= 0.1);

    if (m_searchText != "") {
        if (m_searchMatches) {
            userColour = QColor("#008400");
            showProperLines = true;
            showText = true;
        } else {
            branchColour = Qt::gray;
            userColour = Qt::gray;
        }
    }

    if (!showProperLines) {
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

    int textwid = width - 7;

    QString comment;
    QStringList lines;
    int lineCount = 3;

    if (showText) {
    
        comment = m_changeset->comment().trimmed();
        comment = comment.replace("\\n", " ");
        comment = comment.replace(QRegExp("^\"\\s*\\**\\s*"), "");
        comment = comment.replace(QRegExp("\"$"), "");
        comment = comment.replace("\\\"", "\"");

        comment = TextAbbrev::abbreviate(comment, fm, textwid,
                                         TextAbbrev::ElideEnd, "...", 3);
        // abbreviate() changes this (ouch!), restore it
        textwid = width - 5;

        lines = comment.split('\n');
        lineCount = lines.size();

        if (lineCount < 2) lineCount = 2;
    }

    int height = (lineCount + 1) * fh + 2;
    QRectF r(x0, 0, width - 3, height);

    QColor textColour = Qt::black;
    textColour.setAlpha(alpha);

    if (m_showBranch && showText) {
	// write branch name
        paint->save();
	f.setBold(true);
	paint->setFont(f);
	paint->setPen(QPen(branchColour));
	QString branch = m_changeset->branch();
        if (branch == "") branch = "default";
	int wid = width - 3;
	branch = TextAbbrev::abbreviate(branch, QFontMetrics(f), wid);
	paint->drawText(x0, -fh + fm.ascent() - 4, branch);
	f.setBold(false);
        paint->restore();
    }

    QStringList bookmarks = m_changeset->bookmarks();
    if (!bookmarks.empty() && showText) {
        QString bmText = bookmarks.join(" ").trimmed();
        int bw = fm.width(bmText);
        int bx = x0 + width - bw - 14;
        if (m_current) bx = bx - fh*1.5 + 3;
        paint->save();
        paint->setPen(QPen(branchColour, 2));
//        paint->setBrush(QBrush(Qt::white));
        paint->setBrush(QBrush(branchColour));
        paint->drawRoundedRect(QRectF(bx, -fh - 4, bw + 4, fh * 2), 5, 5);
        paint->setPen(QPen(Qt::white));
        paint->drawText(bx + 2, -fh + fm.ascent() - 4, bmText);
        paint->restore();
    }

    if (showProperLines) {

        if (m_new) {
            paint->setBrush(QColor(255, 255, 220));
        } else {
            paint->setBrush(Qt::white);
        }            

        if (m_current) {
            paint->drawRoundedRect(QRectF(x0 - 4, -4, width + 5, height + 8),
                                   10, 10);
            if (m_new) {
                paint->save();
                paint->setPen(Qt::yellow);
                paint->setBrush(Qt::NoBrush);
                paint->drawRoundedRect(QRectF(x0 - 2, -2, width + 1, height + 4),
                                       10, 10);
                paint->restore();
            }
        }
    }

    if (!showText) {
        paint->drawRoundedRect(r, 7, 7);
	paint->restore();
	return;
    }

    paint->save();
    paint->setPen(Qt::NoPen);
    paint->drawRoundedRect(r, 7, 7);
    paint->setBrush(QBrush(userColour));
    paint->drawRoundedRect(QRectF(x0 + 0.5, 0.5, width - 4, fh - 0.5), 7, 7);
    paint->drawRect(QRectF(x0 + 0.5, fh/2.0, width - 4, fh/2.0));
    paint->restore();

    paint->setPen(QPen(Qt::white));

    QString person = TextAbbrev::abbreviate(m_changeset->authorName(),
                                            fm, textwid);
    paint->drawText(x0 + 3, fm.ascent(), person);

    paint->setPen(QPen(textColour));

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

    paint->setPen(QPen(branchColour, 2));
    paint->setBrush(Qt::NoBrush);
    paint->drawRoundedRect(r, 7, 7);

    if (m_current && showProperLines) {
        paint->setRenderHint(QPainter::SmoothPixmapTransform, true);
        int starSize = fh * 1.5;
        paint->drawImage(QRectF(x0 + width - starSize,
                                -fh, starSize, starSize),
                         *m_star);
    }

    paint->setFont(f);

    if (m_searchMatches) paint->setPen(userColour);

    for (int i = 0; i < lines.size(); ++i) {
	paint->drawText(x0 + 3, i * fh + fh + fm.ascent(), lines[i].trimmed());
    }

    paint->restore();
}

void
ChangesetItem::paintSimple(QPainter *paint)
{
    paint->save();

    int alpha = 255;
    if (isClosed()) alpha = 90;
    
    ColourSet *colourSet = ColourSet::instance();
    QColor branchColour = colourSet->getColourFor(m_changeset->branch());
    QColor userColour = colourSet->getColourFor(m_changeset->author());

    branchColour.setAlpha(alpha);
    userColour.setAlpha(alpha);

    QFont f(m_font);
    QTransform t = paint->worldTransform();
    float scale = std::min(t.m11(), t.m22());
    if (scale > 1.0) {
	int ps = int((f.pixelSize() / scale) + 0.5);
	if (ps < 8) ps = 8;
	f.setPixelSize(ps);
#ifdef Q_OS_WIN32
        f.setHintingPreference(QFont::PreferVerticalHinting);
#endif
    }

    bool showProperLines = (scale >= 0.1);

    if (!showProperLines) {
	paint->setPen(QPen(branchColour, 0));
    } else {
	paint->setPen(QPen(branchColour, 2));
    }
	
    paint->setFont(f);
    QFontMetrics fm(f);
    int fh = fm.height();
    int size = fh * 2;
    int x0 = -size/2 + 25;

    if (m_new) {
        paint->setBrush(QColor(255, 255, 220));
    } else {
        paint->setBrush(Qt::white);
    }

    if (showProperLines) {

        if (m_current) {
            if (isClosingCommit()) {
                paint->drawRect(QRectF(x0 - 4, fh - 4, size + 8, size + 8));
            } else {
                paint->drawEllipse(QRectF(x0 - 4, fh - 4, size + 8, size + 8));
            }
            
            if (m_new) {
                paint->save();
                paint->setPen(Qt::yellow);
                paint->setBrush(Qt::NoBrush);
                if (isClosingCommit()) {
                    paint->drawRect(QRectF(x0 - 2, fh - 2, size + 4, size + 4));
                } else {
                    paint->drawEllipse(QRectF(x0 - 2, fh - 2, size + 4, size + 4));
                }
                paint->restore();
            }
        }
    }

    if (isClosingCommit()) {
        paint->drawRect(QRectF(x0, fh, size, size));
    } else {
        paint->drawEllipse(QRectF(x0, fh, size, size));
    }

    if (m_showBranch) {
	// write branch name
        paint->save();
	f.setBold(true);
	paint->setFont(f);
	paint->setPen(QPen(branchColour));
	QString branch = m_changeset->branch();
        if (branch == "") branch = "default";
	int wid = size * 3;
	branch = TextAbbrev::abbreviate(branch, QFontMetrics(f), wid);
	paint->drawText(-wid/2 + 25, fm.ascent() - 4, branch);
	f.setBold(false);
        paint->restore();
    }

    QStringList tags = m_changeset->tags();
    if (!tags.empty()) {
        QStringList nonTipTags;
        foreach (QString t, tags) {
            if (t != "tip") nonTipTags.push_back(t);
        }
        if (!nonTipTags.empty()) {
            QString tagText = nonTipTags.join(" ").trimmed();
            int tw = fm.width(tagText);
            paint->fillRect(QRectF(x0 + size/2 + 2, 0, tw + 4, fh - 1),
                            QBrush(Qt::yellow));
            paint->drawText(x0 + size/2 + 4, fm.ascent() - 1, tagText);
        }
    }

    if (m_current && showProperLines) {
        paint->setRenderHint(QPainter::SmoothPixmapTransform, true);
        int starSize = fh * 1.5;
        paint->drawImage(QRectF(x0 + size - starSize/2,
                                0, starSize, starSize),
                         *m_star);
    }

    paint->restore();
}

