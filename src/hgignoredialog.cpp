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

#include "hgignoredialog.h"
#include "common.h"
#include "debug.h"

#include <QGridLayout>
#include <QRadioButton>
#include <QLabel>
#include <QDialogButtonBox>
#include <QPushButton>

HgIgnoreDialog::HgIgnoreDialog(QWidget *parent,
			       QString title,
			       QString introText,
			       QString question,
			       QStringList options,
			       QString okButtonText) :
    QDialog(parent)
{
    setWindowTitle(title);

    QGridLayout *layout = new QGridLayout;
    setLayout(layout);
    
    int row = 0;

    QLabel *label = new QLabel(QString("%1%2").arg(introText).arg(question));
    label->setWordWrap(true);
    layout->addWidget(label, row++, 0, 1, 2);

    if (!options.empty()) {
	layout->addWidget(new QLabel("  "), row, 0);
	layout->setColumnStretch(1, 10);
	bool first = true;
	foreach (QString option, options) {
	    QRadioButton *b = new QRadioButton(option);
	    layout->addWidget(b, row++, 1);
	    if (first) {
		m_option = option;
		b->setChecked(true);
		first = false;
	    }
	    connect(b, SIGNAL(toggled(bool)), this, SLOT(optionToggled(bool)));
	}
    }

    QDialogButtonBox *bbox = new QDialogButtonBox(QDialogButtonBox::Ok |
                                                  QDialogButtonBox::Cancel);
    layout->addWidget(bbox, row++, 0, 1, 2);
    bbox->button(QDialogButtonBox::Ok)->setDefault(true);
    bbox->button(QDialogButtonBox::Ok)->setText(okButtonText);
    bbox->button(QDialogButtonBox::Cancel)->setAutoDefault(false);

    connect(bbox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(bbox, SIGNAL(rejected()), this, SLOT(reject()));
}

void
HgIgnoreDialog::optionToggled(bool checked)
{
    QObject *s = sender();
    QRadioButton *rb = qobject_cast<QRadioButton *>(s);
    if (rb && checked) {
	m_option = rb->text();
    }
}

HgIgnoreDialog::IgnoreType
HgIgnoreDialog::confirmIgnore(QWidget *parent,
			      QStringList files,
                              QStringList suffixes,
                              QString directory)
{
    QString intro = "<qt><h3>";
    intro += tr("Ignore files");
    intro += "</h3><p>";

    if (files.size() < 10) {
        intro += tr("You have asked to ignore the following files:</p><p>");
        intro += "<code>&nbsp;&nbsp;&nbsp;"
	    + files.join("<br>&nbsp;&nbsp;&nbsp;") + "</code>";
    } else {
        intro += tr("You have asked to ignore %n file(s).", "", files.size());
    }

    intro += "</p></qt>";

    QString textTheseFiles;
    QString textTheseNames;
    if (files.size() > 1) {
        textTheseFiles = tr("Ignore these files only");
        textTheseNames = tr("Ignore files with these names, in any folder");
    } else {
        textTheseFiles = tr("Ignore this file only");
        textTheseNames = tr("Ignore files with the same name as this, in any folder");
    }        

    QString textThisFolder;
    QString textTheseSuffixes;

    QStringList options;
    options << textTheseFiles;
    options << textTheseNames;

    if (directory != "") {
        textThisFolder = tr("Ignore the whole folder \"%1\"")
            .arg(directory);
        options << textThisFolder;
    }

    if (suffixes.size() > 1) {

        textTheseSuffixes = tr("Ignore all files with these extensions:\n%1")
            .arg(suffixes.join(", "));
        options << textTheseSuffixes;

    } else if (suffixes.size() > 0) {

        textTheseSuffixes = tr("Ignore all files with the extension \"%1\"")
            .arg(suffixes[0]);
        options << textTheseSuffixes;
    }

    HgIgnoreDialog d(parent, tr("Ignore files"),
                     intro, tr("<p>Please choose whether to:</p>"),
                     options, tr("Ignore"));

    if (d.exec() == QDialog::Accepted) {
        QString option = d.getOption();
        DEBUG << "HgIgnoreDialog::confirmIgnore: option = " << option << endl;
        if (option == textTheseFiles) return IgnoreGivenFilesOnly;
        else if (option == textTheseNames) return IgnoreAllFilesOfGivenNames;
        else if (option == textTheseSuffixes) return IgnoreAllFilesOfGivenSuffixes;
        else if (option == textThisFolder) return IgnoreWholeDirectory;
    }

    return IgnoreNothing;
}

