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

#include "filestatuswidget.h"

#include <QLabel>
#include <QListWidget>
#include <QGridLayout>

FileStatusWidget::FileStatusWidget(QWidget *parent) :
    QWidget(parent)
{
    QGridLayout *layout = new QGridLayout;
    setLayout(layout);

    int row = 0;
    
    layout->addWidget(new QLabel(tr("Local:")), row, 0);
    m_localPathLabel = new QLabel;
    layout->addWidget(m_localPathLabel, row, 1);

    ++row;
    layout->addWidget(new QLabel(tr("Remote:")), row, 0);
    m_remoteURLLabel = new QLabel;
    layout->addWidget(m_remoteURLLabel, row, 1);

    m_modifiedList = new QListWidget;
    m_addedList = new QListWidget;
    m_unknownList = new QListWidget;
    m_removedList = new QListWidget;
    m_missingList = new QListWidget;

    layout->addWidget(m_modifiedList, ++row, 0, 1, 2);
    layout->addWidget(m_addedList, ++row, 0, 1, 2);
    layout->addWidget(m_removedList, ++row, 0, 1, 2);
    layout->addWidget(m_unknownList, ++row, 0, 1, 2);
    layout->addWidget(m_missingList, ++row, 0, 1, 2);
}

void
FileStatusWidget::setLocalPath(QString p)
{
    m_localPath = p;
    m_localPathLabel->setText(p);
}

void
FileStatusWidget::setRemoteURL(QString r)
{
    m_remoteURL = r;
    m_remoteURLLabel->setText(r);
}

void
FileStatusWidget::setStatParser(StatParser p)
{
    m_statParser = p;
    updateWidgets();
}

void
FileStatusWidget::updateWidgets()
{
    m_modifiedList->clear();
    m_addedList->clear();
    m_unknownList->clear();
    m_removedList->clear();
    m_missingList->clear();

    m_modifiedList->addItems(m_statParser.modified);
    m_addedList->addItems(m_statParser.added);
    m_unknownList->addItems(m_statParser.unknown);
    m_removedList->addItems(m_statParser.removed);
    m_missingList->addItems(m_statParser.missing);
}

