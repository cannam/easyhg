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
#include <QFileInfo>
#include <QApplication>
#include <QDateTime>

#include "debug.h"

FileStatusWidget::FileStatusWidget(QWidget *parent) :
    QWidget(parent),
    m_dateReference(0)
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

    QMap<FileStates::State, QString> labels;
    labels[FileStates::Clean] = tr("Unmodified files:");
    labels[FileStates::Modified] = tr("Modified files:");
    labels[FileStates::Added] = tr("Added files:");
    labels[FileStates::Removed] = tr("Removed files:");
    labels[FileStates::Missing] = tr("Missing files:");
    labels[FileStates::Unknown] = tr("Untracked files:");

    for (int i = int(FileStates::FirstState);
             i <= int(FileStates::LastState); ++i) {

        FileStates::State s = FileStates::State(i);

        QWidget *box = new QWidget;
        QGridLayout *boxlayout = new QGridLayout;
        box->setLayout(boxlayout);

        boxlayout->addWidget(new QLabel(labels[s]), 0, 0);

        QListWidget *w = new QListWidget;
        m_stateListMap[s] = w;
        w->setSelectionMode(QListWidget::ExtendedSelection);
        boxlayout->addWidget(w, 1, 0);

        layout->addWidget(box, ++row, 0, 1, 2);
        box->hide();
    }

    layout->setRowStretch(++row, 20);
}

FileStatusWidget::~FileStatusWidget()
{
    delete m_dateReference;
}

void FileStatusWidget::clearSelections()
{
    foreach (QListWidget *w, m_stateListMap) {
        w->clearSelection();
    }
}

void
FileStatusWidget::setLocalPath(QString p)
{
    m_localPath = p;
    m_localPathLabel->setText(p);
    delete m_dateReference;
    m_dateReference = new QFileInfo(p + "/.hg/dirstate");
    if (!m_dateReference->exists() ||
        !m_dateReference->isFile() ||
        !m_dateReference->isReadable()) {
        DEBUG << "FileStatusWidget::setLocalPath: date reference file "
                << m_dateReference->absoluteFilePath()
                << " does not exist, is not a file, or cannot be read"
                << endl;
        delete m_dateReference;
        m_dateReference = 0;
    }
}

void
FileStatusWidget::setRemoteURL(QString r)
{
    m_remoteURL = r;
    m_remoteURLLabel->setText(r);
}

void
FileStatusWidget::setFileStates(FileStates p)
{
    m_fileStates = p;
    updateWidgets();
}

void
FileStatusWidget::highlightFile(QListWidget *w, int i)
{
    DEBUG << "FileStatusWidget: highlighting file at " << i << endl;
    QListWidgetItem *item = w->item(i);
    item->setForeground(Qt::red);
    //!!! and a nice gold star
}

void
FileStatusWidget::updateWidgets()
{
    foreach (FileStates::State s, m_stateListMap.keys()) {
        QListWidget *w = m_stateListMap[s];
        w->clear();
        QStringList sl = m_fileStates.getFilesInState(s);
        w->addItems(sl);
        w->parentWidget()->setVisible(!sl.empty());
    }

    if (m_dateReference) {
        // Highlight untracked files that have appeared since the
        // last interaction with the repo
        QDateTime refTime = m_dateReference->lastModified();
        DEBUG << "reference time: " << refTime << endl;
        QListWidget *ul = m_stateListMap[FileStates::Unknown];
        for (int i = 0; i < ul->count(); ++i) {
            QString fn(m_localPath + "/" + ul->item(i)->text());
            DEBUG << "comparing with " << fn << endl;
            QFileInfo fi(fn);
            if (fi.exists() && fi.lastModified() > refTime) {
                DEBUG << "file " << fn << " is newer (" << fi.lastModified()
                        << ") than reference" << endl;
                highlightFile(ul, i);
            }
        }
    }
}

