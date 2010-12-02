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

#include "hgtabwidget.h"
#include "common.h"
#include "filestatuswidget.h"
#include "historywidget.h"

#include <QClipboard>
#include <QContextMenuEvent>
#include <QApplication>

#include <iostream>

HgTabWidget::HgTabWidget(QWidget *parent,
                         QString remoteRepo,
                         QString workFolderPath) :
    QTabWidget(parent)
{
    // Work page
    m_fileStatusWidget = new FileStatusWidget;
    m_fileStatusWidget->setLocalPath(workFolderPath);
    m_fileStatusWidget->setRemoteURL(remoteRepo);
    connect(m_fileStatusWidget, SIGNAL(selectionChanged()),
            this, SIGNAL(selectionChanged()));
    addTab(m_fileStatusWidget, tr("My work"));

    // History graph page
    m_historyWidget = new HistoryWidget;
    addTab(m_historyWidget, tr("History"));

    connect(m_historyWidget, SIGNAL(commit()),
            this, SIGNAL(commit()));
    
    connect(m_historyWidget, SIGNAL(revert()),
            this, SIGNAL(revert()));
    
    connect(m_historyWidget, SIGNAL(diffWorkingFolder()),
            this, SIGNAL(diffWorkingFolder()));

    connect(m_historyWidget, SIGNAL(updateTo(QString)),
            this, SIGNAL(updateTo(QString)));

    connect(m_historyWidget, SIGNAL(diffToCurrent(QString)),
            this, SIGNAL(diffToCurrent(QString)));

    connect(m_historyWidget, SIGNAL(diffToParent(QString, QString)),
            this, SIGNAL(diffToParent(QString, QString)));

    connect(m_historyWidget, SIGNAL(mergeFrom(QString)),
            this, SIGNAL(mergeFrom(QString)));

    connect(m_historyWidget, SIGNAL(tag(QString)),
            this, SIGNAL(tag(QString)));
}

void HgTabWidget::clearSelections()
{
    m_fileStatusWidget->clearSelections();
}

void HgTabWidget::setCurrent(QStringList ids)
{
    m_historyWidget->setCurrent(ids, canCommit());
}

bool HgTabWidget::canCommit() const
{
    if (!m_fileStatusWidget->getSelectedAddableFiles().empty()) return false;
    return m_fileStatusWidget->haveChangesToCommit();
}

bool HgTabWidget::canRevert() const
{
    return m_fileStatusWidget->haveChangesToCommit() ||
        !m_fileStatusWidget->getSelectedRevertableFiles().empty();
}

bool HgTabWidget::canAdd() const
{
    if (m_fileStatusWidget->getSelectedAddableFiles().empty()) return false;
    if (!m_fileStatusWidget->getSelectedCommittableFiles().empty()) return false;
    if (!m_fileStatusWidget->getSelectedRemovableFiles().empty()) return false;
    return true;
}

bool HgTabWidget::canRemove() const
{
    if (m_fileStatusWidget->getSelectedRemovableFiles().empty()) return false;
    if (!m_fileStatusWidget->getSelectedAddableFiles().empty()) return false;
    return true;
}

bool HgTabWidget::canDoDiff() const
{
    return canCommit();
}

QStringList HgTabWidget::getAllSelectedFiles() const
{
    return m_fileStatusWidget->getAllSelectedFiles();
}

QStringList HgTabWidget::getAllCommittableFiles() const
{
    return m_fileStatusWidget->getAllCommittableFiles();
}

QStringList HgTabWidget::getSelectedCommittableFiles() const
{
    return m_fileStatusWidget->getSelectedCommittableFiles();
}

QStringList HgTabWidget::getAllRevertableFiles() const
{
    return m_fileStatusWidget->getAllRevertableFiles();
}

QStringList HgTabWidget::getSelectedRevertableFiles() const
{
    return m_fileStatusWidget->getSelectedRevertableFiles();
}

QStringList HgTabWidget::getSelectedAddableFiles() const
{
    return m_fileStatusWidget->getSelectedAddableFiles();
}

QStringList HgTabWidget::getAllRemovableFiles() const
{
    return m_fileStatusWidget->getAllRemovableFiles();
}

QStringList HgTabWidget::getSelectedRemovableFiles() const
{
    return m_fileStatusWidget->getSelectedRemovableFiles();
}

void HgTabWidget::updateWorkFolderFileList(QString fileList)
{
    m_fileStates.parseStates(fileList);
    m_fileStatusWidget->setFileStates(m_fileStates);
}

void HgTabWidget::setNewLog(QString hgLogList)
{
    m_historyWidget->parseNewLog(hgLogList);
    if (m_historyWidget->haveNewItems()) {
        setCurrentWidget(m_historyWidget);
    }
}

void HgTabWidget::addIncrementalLog(QString hgLogList)
{
    m_historyWidget->parseIncrementalLog(hgLogList);
    if (m_historyWidget->haveNewItems()) {
        setCurrentWidget(m_historyWidget);
    }
}

void HgTabWidget::setWorkFolderAndRepoNames(QString workFolderPath, QString remoteRepoPath)
{
    m_fileStatusWidget->setLocalPath(workFolderPath);
    m_fileStatusWidget->setRemoteURL(remoteRepoPath);
}

void HgTabWidget::setState(QString state)
{
    m_fileStatusWidget->setState(state);
}
