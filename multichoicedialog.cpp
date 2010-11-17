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

#include "multichoicedialog.h"

#include <QDialogButtonBox>

MultiChoiceDialog::MultiChoiceDialog(QString title, QString heading, QWidget *parent) :
    QDialog(parent)
{
    setModal(true);
    setWindowTitle(title);

    QGridLayout *outer = new QGridLayout;
    setLayout(outer);

    outer->addWidget(new QLabel(heading), 0, 0, 1, 3);

    QWidget *innerWidget = new QWidget;
    outer->addWidget(innerWidget, 1, 0, 1, 2);
    m_choiceLayout = new QGridLayout;
    innerWidget->setLayout(m_choiceLayout);

    m_descriptionLabel = new QLabel;
    outer->addWidget(m_descriptionLabel, 2, 0, 1, 3);

    m_argLabel = new QLabel();
    outer->addWidget(m_argLabel, 3, 0);

    m_argEdit = new QLineEdit();
    outer->addWidget(m_argEdit, 3, 1);

    m_browseButton = new QPushButton(tr("Browse..."));
    outer->addWidget(m_browseButton, 3, 2);

    QDialogButtonBox *bbox = new QDialogButtonBox(QDialogButtonBox::Ok |
                                                  QDialogButtonBox::Cancel);
    connect(bbox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(bbox, SIGNAL(rejected()), this, SLOT(reject()));
    outer->addWidget(bbox, 4, 0, 1, 3);
}

