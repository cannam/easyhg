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

#include "annotatedialog.h"
#include "common.h"
#include "debug.h"

#include <QDialogButtonBox>
#include <QLabel>
#include <QTableWidget>
#include <QHeaderView>
#include <QGridLayout>

AnnotateDialog::AnnotateDialog(QWidget *w, QString text) :
    QDialog(w)
{
    setMinimumWidth(600);
    setMinimumHeight(700);

    text.replace("\r\n", "\n");
    QStringList lines = text.split("\n");

    QGridLayout *layout = new QGridLayout;
    QTableWidget *table = new QTableWidget;

    QRegExp annotateLineRE = QRegExp("^([^:]+) ([a-z0-9]{12}) ([0-9-]+): (.*)$");

    table->setRowCount(lines.size());
    table->setColumnCount(4);
    table->horizontalHeader()->setStretchLastSection(true);

    QStringList labels;
    labels << tr("User") << tr("Revision") << tr("Date") << tr("Content");
    table->setHorizontalHeaderLabels(labels);

    int row = 0;

    foreach (QString line, lines) {
	if (annotateLineRE.indexIn(line) == 0) {
	    QStringList items = annotateLineRE.capturedTexts();
	    // note items[0] is the whole match, so we want 1-4
	    for (int col = 0; col+1 < items.size(); ++col) {
		std::cerr << "row " << row << " col " << col << " text "
			  << items[col+1] << std::endl;
		table->setItem(row, col, new QTableWidgetItem(items[col+1]));
	    }
	} else {
	    DEBUG << "AnnotateDialog: Failed to match RE in line: " << line << " at row " << row << endl;
	}
	++row;
    }

    QDialogButtonBox *bb = new QDialogButtonBox(QDialogButtonBox::Ok);
    connect(bb, SIGNAL(accepted()), this, SLOT(accept()));

    layout->addWidget(table, 0, 0);
    layout->addWidget(bb, 1, 0);

    setLayout(layout);
}

