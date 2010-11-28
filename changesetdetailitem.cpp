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

#include "changesetdetailitem.h"
#include "changeset.h"
#include "textabbrev.h"
#include "colourset.h"
#include "debug.h"
#include "common.h"

#include <QTextDocument>
#include <QPainter>

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
    return QRectF(0, -10, w, m_doc->size().height() + 10);
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

    int width = 350;
    m_doc->setTextWidth(width);
    int height = m_doc->size().height();

    QRectF r(0.5, 0.5, width - 1, height - 1);
    paint->setBrush(Qt::white);
    paint->drawRect(r);

    if (scale < 0.1) {
	paint->restore();
	return;
    }

    paint->setBrush(branchColour);
    QVector<QPointF> pts;
    pts.push_back(QPointF(width/2 - 5, 0));
    pts.push_back(QPointF(width/2 + 5, 0));
    pts.push_back(QPointF(width/2, -10));
    pts.push_back(QPointF(width/2 - 5, 0));
    paint->drawPolygon(QPolygonF(pts));

    m_doc->drawContents(paint, r);

/*
    paint->fillRect(QRectF(x0 + 0.5, 0.5, width - 4, fh - 0.5),
		    QBrush(userColour));

    paint->setPen(QPen(Qt::white));

    int wid = width - 5;
    QString person = TextAbbrev::abbreviate(m_changeset->authorName(), fm, wid);
    paint->drawText(x0 + 3, fm.ascent(), person);

    paint->setPen(QPen(Qt::black));

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

    wid = width - 5;
    int nlines = (height / fh) - 1;
    if (nlines < 1) nlines = 1;
    comment = TextAbbrev::abbreviate(comment, fm, wid, TextAbbrev::ElideEnd,
				     "...", nlines);

    QStringList lines = comment.split('\n');
    for (int i = 0; i < lines.size(); ++i) {
	paint->drawText(x0 + 3, i * fh + fh + fm.ascent(), lines[i].trimmed());
    }
    */
    paint->restore();
}

void
ChangesetDetailItem::makeDocument()
{
    delete m_doc;

    QString description;
    QString rowTemplate = "<tr><td><b>%1</b></td><td>%2</td></tr>";

    description = "<qt><table border=0>";

    QString comment = m_changeset->comment().trimmed();
    comment = comment.replace(QRegExp("^\""), "");
    comment = comment.replace(QRegExp("\"$"), "");
    comment = comment.replace("\\\"", "\"");
    comment = xmlEncode(comment);
    comment = comment.replace("\\n", "<br>");

    QStringList propNames, propTexts;
    
    propNames << "id"
	      << "author"
	      << "datetime"
	      << "branch"
	      << "tag"
	      << "comment";

    propTexts << QObject::tr("Identifier")
	      << QObject::tr("Author")
	      << QObject::tr("Date")
	      << QObject::tr("Branch")
	      << QObject::tr("Tag")
	      << QObject::tr("Comment");

    for (int i = 0; i < propNames.size(); ++i) {
	QString prop = propNames[i];
	QString value;
	if (prop == "comment") value = comment;
	else {
	    value = xmlEncode(m_changeset->property
			      (prop.toLocal8Bit().data()).toString());
	}
	if (value != "") {
	    description += rowTemplate
		.arg(xmlEncode(propTexts[i]))
		.arg(value);
	}
    }

    description += "</table></qt>";

    DEBUG << "ChangesetDetailItem: description = " << description << endl;

    m_doc = new QTextDocument;
    m_doc->setHtml(description);
}
