/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    EasyMercurial

    Based on hgExplorer by Jari Korhonen
    Copyright (c) 2010 Jari Korhonen
    Copyright (c) 2011 Chris Cannam
    Copyright (c) 2011 Queen Mary, University of London

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "incomingdialog.h"
#include "changeset.h"
#include "common.h"

#include <QScrollArea>
#include <QApplication>
#include <QDialogButtonBox>
#include <QLabel>
#include <QGridLayout>
#include <QStyle>

IncomingDialog::IncomingDialog(QWidget *w, QString text) :
    QDialog(w)
{
    QString head;
    QString body;
    bool scroll;

    Changesets csets = Changeset::parseChangesets(text);
    if (csets.empty()) {
	head = tr("No changes waiting to pull");
	if (text.trimmed() != "") {
	    body = QString("<p>%1</p><code>%2</code>")
		.arg(tr("The command output was:"))
		.arg(xmlEncode(text).replace("\n", "<br>"));
	} else {
            body = tr("<qt>Your local repository already contains all changes found in the remote repository.</qt>");
        }
	scroll = false;
    } else {
        head = tr("There are %n change(s) ready to pull", "", csets.size());
	foreach (Changeset *cs, csets) {
	    body += cs->formatHtml() + "<p>";
	    delete cs;
	}
	scroll = true;
    }

    QGridLayout *layout = new QGridLayout;
    setLayout(layout);

    QLabel *info = new QLabel;
    QStyle *style = qApp->style();
    int iconSize = style->pixelMetric(QStyle::PM_MessageBoxIconSize, 0, this);
    info->setPixmap(style->standardIcon(QStyle::SP_MessageBoxInformation, 0, this)
		    .pixmap(iconSize, iconSize));
    layout->addWidget(info, 0, 0, 2, 1);

    QLabel *headLabel = new QLabel(QString("<qt><h3>%1</h3></qt>").arg(head));
    layout->addWidget(headLabel, 0, 1);

    QLabel *textLabel = new QLabel(body);
    if (csets.empty()) textLabel->setWordWrap(true);

    if (scroll) {
	QScrollArea *sa = new QScrollArea;
	layout->addWidget(sa, 1, 1);
	layout->setRowStretch(1, 20);
	sa->setWidget(textLabel);
    } else {
	layout->addWidget(textLabel, 1, 1);
    }

    QDialogButtonBox *bb = new QDialogButtonBox(QDialogButtonBox::Ok);
    connect(bb, SIGNAL(accepted()), this, SLOT(accept()));
    layout->addWidget(bb, 2, 0, 1, 2);
}

    
