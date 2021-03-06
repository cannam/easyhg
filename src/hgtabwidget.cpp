/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    EasyMercurial

    Based on HgExplorer by Jari Korhonen
    Copyright (c) 2010 Jari Korhonen
    Copyright (c) 2013 Chris Cannam
    Copyright (c) 2013 Queen Mary, University of London
    
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
#include <QSettings>

#include <iostream>

HgTabWidget::HgTabWidget(QWidget *parent,
                         QString workFolderPath) :
    QTabWidget(parent),
    m_haveMerge(false)
{
    // Work tab
    m_fileStatusWidget = new FileStatusWidget;
    m_fileStatusWidget->setLocalPath(workFolderPath);

    connect(m_fileStatusWidget, SIGNAL(selectionChanged()),
            this, SIGNAL(selectionChanged()));

    connect(m_fileStatusWidget, SIGNAL(showAllChanged()),
            this, SIGNAL(showAllChanged()));

    connect(m_fileStatusWidget, SIGNAL(annotateFiles(QStringList)),
            this, SIGNAL(annotateFiles(QStringList)));

    connect(m_fileStatusWidget, SIGNAL(diffFiles(QStringList)),
            this, SIGNAL(diffFiles(QStringList)));

    connect(m_fileStatusWidget, SIGNAL(commitFiles(QStringList)),
            this, SIGNAL(commitFiles(QStringList)));

    connect(m_fileStatusWidget, SIGNAL(revertFiles(QStringList)),
            this, SIGNAL(revertFiles(QStringList)));

    connect(m_fileStatusWidget, SIGNAL(renameFiles(QStringList)),
            this, SIGNAL(renameFiles(QStringList)));

    connect(m_fileStatusWidget, SIGNAL(copyFiles(QStringList)),
            this, SIGNAL(copyFiles(QStringList)));

    connect(m_fileStatusWidget, SIGNAL(addFiles(QStringList)),
            this, SIGNAL(addFiles(QStringList)));

    connect(m_fileStatusWidget, SIGNAL(removeFiles(QStringList)),
            this, SIGNAL(removeFiles(QStringList)));

    connect(m_fileStatusWidget, SIGNAL(redoFileMerges(QStringList)),
            this, SIGNAL(redoFileMerges(QStringList)));

    connect(m_fileStatusWidget, SIGNAL(markFilesResolved(QStringList)),
            this, SIGNAL(markFilesResolved(QStringList)));

    connect(m_fileStatusWidget, SIGNAL(ignoreFiles(QStringList)),
            this, SIGNAL(ignoreFiles(QStringList)));

    connect(m_fileStatusWidget, SIGNAL(unIgnoreFiles(QStringList)),
            this, SIGNAL(unIgnoreFiles(QStringList)));

    connect(m_fileStatusWidget, SIGNAL(showIn(QStringList)),
            this, SIGNAL(showIn(QStringList)));

    addTab(m_fileStatusWidget, tr("My work"));

    // History graph tab
    m_historyWidget = new HistoryWidget;
    addTab(m_historyWidget, tr("History"));

    connect(m_historyWidget, SIGNAL(commit()),
            this, SIGNAL(commit()));
    
    connect(m_historyWidget, SIGNAL(revert()),
            this, SIGNAL(revert()));
    
    connect(m_historyWidget, SIGNAL(showSummary()),
            this, SIGNAL(showSummary()));
    
    connect(m_historyWidget, SIGNAL(newBranch()),
            this, SIGNAL(newBranch()));
    
    connect(m_historyWidget, SIGNAL(noBranch()),
            this, SIGNAL(noBranch()));
    
    connect(m_historyWidget, SIGNAL(diffWorkingFolder()),
            this, SIGNAL(diffWorkingFolder()));

    connect(m_historyWidget, SIGNAL(showWork()),
            this, SLOT(showWorkTab()));

    connect(m_historyWidget, SIGNAL(updateTo(QString)),
            this, SIGNAL(updateTo(QString)));

    connect(m_historyWidget, SIGNAL(diffToCurrent(QString)),
            this, SIGNAL(diffToCurrent(QString)));

    connect(m_historyWidget, SIGNAL(diffToParent(QString, QString)),
            this, SIGNAL(diffToParent(QString, QString)));

    connect(m_historyWidget, SIGNAL(showSummary(Changeset *)),
            this, SIGNAL(showSummary(Changeset *)));

    connect(m_historyWidget, SIGNAL(mergeFrom(QString)),
            this, SIGNAL(mergeFrom(QString)));

    connect(m_historyWidget, SIGNAL(newBranch(QString)),
            this, SIGNAL(newBranch(QString)));

    connect(m_historyWidget, SIGNAL(closeBranch(QString)),
            this, SIGNAL(closeBranch(QString)));

    connect(m_historyWidget, SIGNAL(tag(QString)),
            this, SIGNAL(tag(QString)));
}

void HgTabWidget::clearSelections()
{
    m_fileStatusWidget->clearSelections();
}

void HgTabWidget::setCurrent(QStringList ids, QString branch)
{
    m_historyWidget->setCurrent(ids, branch, haveChangesToCommit());
}

void HgTabWidget::setClosedHeadIds(QSet<QString> closed)
{
    m_historyWidget->setClosedHeadIds(closed);
}

void HgTabWidget::clearAll()
{
    m_fileStatusWidget->clearWidgets();
    m_historyWidget->clear();
}

void HgTabWidget::updateFileStates()
{
    m_fileStatusWidget->updateWidgets();
}

void HgTabWidget::updateHistory()
{
    m_historyWidget->update();
}

void HgTabWidget::setSearchText(QString text)
{
    m_fileStatusWidget->setSearchText(text);
    m_historyWidget->setSearchText(text);
}

bool HgTabWidget::canDiff() const
{
    return canRevert();
}

bool HgTabWidget::canCommit() const
{
    if (!haveChangesToCommit()) return false;
    if (!getAllUnresolvedFiles().empty()) return false;
    return true;
}

bool HgTabWidget::canRevert() const
{
    // Not the same as canCommit() -- we can revert (and diff)
    // unresolved files, but we can't commit them
    if (!haveChangesToCommit() &&
        getAllUnresolvedFiles().empty()) return false;
    return true;
}

bool HgTabWidget::canAdd() const
{
    // Permit this only when work tab is visible
    if (currentIndex() != 0) return false;

    QStringList addable = getSelectedAddableFiles();
    if (addable.empty()) return false;

    QStringList removable = getSelectedRemovableFiles();
    if (!removable.empty()) return false;

    return true;
}

bool HgTabWidget::canRemove() const
{
    // Permit this only when work tab is visible
    if (currentIndex() != 0) return false;

    if (getSelectedRemovableFiles().empty()) return false;
    if (!getSelectedAddableFiles().empty()) return false;
    return true;
}

bool HgTabWidget::canResolve() const
{
    return !getAllUnresolvedFiles().empty();
}

bool HgTabWidget::canIgnore() const
{
    return canAdd();
}

bool HgTabWidget::haveChangesToCommit() const
{
    return m_haveMerge || m_fileStatusWidget->haveChangesToCommit();
}

QStringList HgTabWidget::getAllCommittableFiles() const
{
    return m_fileStatusWidget->getAllCommittableFiles();
}

QStringList HgTabWidget::getAllRevertableFiles() const
{
    return m_fileStatusWidget->getAllRevertableFiles();
}

QStringList HgTabWidget::getSelectedAddableFiles() const
{
    return m_fileStatusWidget->getSelectedAddableFiles();
}

QStringList HgTabWidget::getSelectedRemovableFiles() const
{
    return m_fileStatusWidget->getSelectedRemovableFiles();
}

QStringList HgTabWidget::getAllUnresolvedFiles() const
{
    return m_fileStatusWidget->getAllUnresolvedFiles();
}

void HgTabWidget::updateWorkFolderFileList(QString fileList)
{
    m_fileStates.parseStates(fileList);
    m_fileStatusWidget->setFileStates(m_fileStates);
}

void HgTabWidget::setHaveMerge(bool haveMerge)
{
    if (m_haveMerge != haveMerge) {
        m_haveMerge = haveMerge;
        m_historyWidget->setShowUncommitted(haveChangesToCommit());
        updateHistory();
    }
}

void HgTabWidget::setNewLog(QString hgLogList)
{
    m_historyWidget->parseNewLog(hgLogList);
    if (m_historyWidget->haveNewItems()) {
        showHistoryTab();
    }
}

void HgTabWidget::addIncrementalLog(QString hgLogList)
{
    m_historyWidget->parseIncrementalLog(hgLogList);
    if (m_historyWidget->haveNewItems()) {
        QSettings settings;
        settings.beginGroup("Presentation");
        if (settings.value("showHistoryAutomatically", true).toBool()) {
            showHistoryTab();
        }
    }
}

void HgTabWidget::setLocalPath(QString workFolderPath)
{
    m_fileStatusWidget->setLocalPath(workFolderPath);
}

void HgTabWidget::showWorkTab()
{
    setCurrentWidget(m_fileStatusWidget);
}

void HgTabWidget::showHistoryTab()
{
    setCurrentWidget(m_historyWidget);
}

bool HgTabWidget::shouldShowAll() const
{
    return m_fileStatusWidget->shouldShowAll();
}

