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

#ifndef HGTABWIDGET_H
#define HGTABWIDGET_H

#include "changeset.h"
#include "common.h"
#include "filestates.h"

#include <QMenu>
#include <QListWidget>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QCheckBox>
#include <QLabel>
#include <QTabWidget>

class FileStatusWidget;
class HistoryWidget;

class HgTabWidget: public QTabWidget
{
    Q_OBJECT

public:
    HgTabWidget(QWidget *parent, QString remoteRepo, QString workFolderPath);

    void updateWorkFolderFileList(QString fileList);

    void setNewLog(QString hgLogList);
    void addIncrementalLog(QString hgLogList);

    void setWorkFolderAndRepoNames(QString workFolderPath, QString remoteRepoPath);
    void setState(QString state);

    void setCurrent(QStringList ids);
    void showUncommittedChanges(bool);

    FileStates getFileStates() { return m_fileStates; }

    bool canCommit() const;
    bool canRevert() const;
    bool canAdd() const;
    bool canRemove() const;
    bool canDoDiff() const;

    QStringList getAllSelectedFiles() const;

    QStringList getSelectedCommittableFiles() const;
    QStringList getAllCommittableFiles() const;

    QStringList getSelectedRevertableFiles() const;
    QStringList getAllRevertableFiles() const;

    QStringList getSelectedAddableFiles() const;
    QStringList getAllAddableFiles() const;

    QStringList getSelectedRemovableFiles() const;
    QStringList getAllRemovableFiles() const;

signals:
    void selectionChanged();

public slots:
    void clearSelections();

private:
    FileStatusWidget *m_fileStatusWidget;
    HistoryWidget *m_historyWidget;
    FileStates m_fileStates;

    Changesets parseChangeSets(QString changeSetsStr);
};

#endif // HGTABWIDGET_H
