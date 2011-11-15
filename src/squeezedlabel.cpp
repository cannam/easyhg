/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    EasyMercurial

    Based on HgExplorer by Jari Korhonen
    Copyright (c) 2010 Jari Korhonen
    Copyright (c) 2011 Chris Cannam
    Copyright (c) 2011 Queen Mary, University of London
    
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

/*
    This file adapted from Rosegarden, a sequencer and musical
    notation editor.  Copyright 2000-2011 the Rosegarden development
    team.

    Adapted from KDE 4.2.0, this code originally Copyright (c) 2000
    Ronny Standtke.
*/

#include "squeezedlabel.h"

#include <iostream>

#include <QContextMenuEvent>
#include <QAction>
#include <QMenu>
#include <QClipboard>
#include <QApplication>
#include <QMimeData>
#include <QDesktopWidget>


class SqueezedLabelPrivate
{
public:
};

void SqueezedLabel::_k_copyFullText()
{
    QMimeData* data = new QMimeData;
    data->setText(fullText);
    QApplication::clipboard()->setMimeData(data);
}

SqueezedLabel::SqueezedLabel(const QString &text , QWidget *parent)
        : QLabel (parent)
{
    setObjectName("SQUEEZED");
    setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));
    fullText = text;
    elideMode = Qt::ElideMiddle;
    squeezeTextToLabel();
}

SqueezedLabel::SqueezedLabel(QWidget *parent)
        : QLabel (parent)
{
    setObjectName("SQUEEZED");
    setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));
    elideMode = Qt::ElideMiddle;
}

SqueezedLabel::~SqueezedLabel()
{
}

void SqueezedLabel::resizeEvent(QResizeEvent *)
{
    squeezeTextToLabel();
}

QSize SqueezedLabel::minimumSizeHint() const
{
    QSize sh = QLabel::minimumSizeHint();
    sh.setWidth(-1);
    return sh;
}

QSize SqueezedLabel::sizeHint() const
{
    int dw = QApplication::desktop()->availableGeometry(QPoint(0, 0)).width();
    int maxWidth = dw * 3 / 4;
    QFontMetrics fm(fontMetrics());
    int textWidth = fm.width(fullText);
    if (textWidth > maxWidth) {
        textWidth = maxWidth;
    }
    return QSize(textWidth, QLabel::sizeHint().height());
}

void SqueezedLabel::setText(const QString &text)
{
    fullText = text;
    squeezeTextToLabel();
}

void SqueezedLabel::clear() {
    fullText.clear();
    QLabel::clear();
}

void SqueezedLabel::squeezeTextToLabel() {
    QFontMetrics fm(fontMetrics());
    int labelWidth = size().width();
    QStringList squeezedLines;
    bool squeezed = false;
    Q_FOREACH(const QString& line, fullText.split('\n')) {
        int lineWidth = fm.width(line);
        if (lineWidth > labelWidth) {
            squeezed = true;
            squeezedLines << fm.elidedText(line, elideMode, labelWidth);
        } else {
            squeezedLines << line;
        }
    }

    if (squeezed) {
        QLabel::setText(squeezedLines.join("\n"));
        setToolTip(fullText);
    } else {
        QLabel::setText(fullText);
        setToolTip(QString());
    }
}

void SqueezedLabel::setAlignment(Qt::Alignment alignment)
{
    // save fullText and restore it
    QString tmpFull(fullText);
    QLabel::setAlignment(alignment);
    fullText = tmpFull;
}

Qt::TextElideMode SqueezedLabel::textElideMode() const
{
    return elideMode;
}

void SqueezedLabel::setTextElideMode(Qt::TextElideMode mode)
{
    elideMode = mode;
    squeezeTextToLabel();
}

void SqueezedLabel::contextMenuEvent(QContextMenuEvent* ev)
{
    // "We" means the KDE team here.
    //
    // We want to reimplement "Copy" to include the elided text.
    // But this means reimplementing the full popup menu, so no more
    // copy-link-address or copy-selection support anymore, since we
    // have no access to the QTextDocument.
    // Maybe we should have a boolean flag in SqueezedLabel itself for
    // whether to show the "Copy Full Text" custom popup?
    // For now I chose to show it when the text is squeezed; when it's not, the
    // standard popup menu can do the job (select all, copy).

    const bool squeezed = text() != fullText;
    const bool showCustomPopup = squeezed;
    if (showCustomPopup) {
        QMenu menu(this);

        QAction* act = new QAction(tr("&Copy Full Text"), this);
        connect(act, SIGNAL(triggered()), this, SLOT(_k_copyFullText()));
        menu.addAction(act);

        ev->accept();
        menu.exec(ev->globalPos());
    } else {
        QLabel::contextMenuEvent(ev);
    }
}

