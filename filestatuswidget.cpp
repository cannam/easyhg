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
    QFont f(m_localPathLabel->font());
    f.setBold(true);
    m_localPathLabel->setFont(f);
    layout->addWidget(m_localPathLabel, row, 1);

    ++row;
    layout->addWidget(new QLabel(tr("Remote:")), row, 0);
    m_remoteURLLabel = new QLabel;
    layout->addWidget(m_remoteURLLabel, row, 1);

    layout->setColumnStretch(1, 20);

    QStringList labels;
    labels << tr("Modified files:")
            << tr("Added files:")
            << tr("Removed files:")
            << tr("New untracked files:")
            << tr("Missing files:");

    QList<QListWidget **> lists;
    lists << &m_modifiedList
            << &m_addedList
            << &m_removedList
            << &m_unknownList
            << &m_missingList;

    for (int i = 0; i < labels.size(); ++i) {

        QWidget *box = new QWidget;
        QGridLayout *boxlayout = new QGridLayout;
        box->setLayout(boxlayout);

        boxlayout->addWidget(new QLabel(labels[i]), 0, 0);

        *lists[i] = new QListWidget;
        (*lists[i])->setSelectionMode(QListWidget::ExtendedSelection);
        boxlayout->addWidget(*lists[i], 1, 0);

        layout->addWidget(box, ++row, 0, 1, 2);
        box->hide();
    }

    layout->setRowStretch(++row, 20);
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
    StatParser &sp = m_statParser;
    QMap<QStringList *, QListWidget *> listmap;
    listmap[&sp.modified] = m_modifiedList;
    listmap[&sp.added] = m_addedList;
    listmap[&sp.removed] = m_removedList;
    listmap[&sp.missing] = m_missingList;
    listmap[&sp.unknown] = m_unknownList;

    foreach (QStringList *sl, listmap.keys()) {
        listmap[sl]->clear();
        listmap[sl]->addItems(*sl);
        listmap[sl]->parentWidget()->setVisible(!sl->empty());
    }
}

