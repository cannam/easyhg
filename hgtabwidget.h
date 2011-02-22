/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    EasyMercurial

    Based on HgExplorer by Jari Korhonen
    Copyright (c) 2010 Jari Korhonen
    Copyright (c) 2011 Chris Cannam
    Copyright (c) 2011 Queen Mary, University of London
    
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
    HgTabWidget(QWidget *parent, QString workFolderPath);

    void updateWorkFolderFileList(QString fileList);

    void setNewLog(QString hgLogList);
    void addIncrementalLog(QString hgLogList);

    void setLocalPath(QString workFolderPath);

    void setCurrent(QStringList ids, QString branch);

    void updateFileStates();
    void updateHistory();

    FileStates getFileStates() { return m_fileStates; }

    bool canDiff() const;
    bool canCommit() const;
    bool canRevert() const;
    bool canAdd() const;
    bool canRemove() const;
    bool canResolve() const;
    bool haveChangesToCommit() const;

    QStringList getAllSelectedFiles() const;

    QStringList getSelectedCommittableFiles() const;
    QStringList getAllCommittableFiles() const;

    QStringList getSelectedRevertableFiles() const;
    QStringList getAllRevertableFiles() const;

    QStringList getSelectedAddableFiles() const;
    QStringList getAllAddableFiles() const;

    QStringList getSelectedRemovableFiles() const;
    QStringList getAllRemovableFiles() const;

    QStringList getSelectedUnresolvedFiles() const;
    QStringList getAllUnresolvedFiles() const;

signals:
    void selectionChanged();
    void showAllChanged(bool);

    void commit();
    void revert();
    void diffWorkingFolder();
    void showSummary();

    void updateTo(QString id);
    void diffToParent(QString id, QString parent);
    void showSummary(Changeset *);
    void diffToCurrent(QString id);
    void mergeFrom(QString id);
    void tag(QString id);

public slots:
    void clearSelections();
    void showWorkTab();
    void showHistoryTab();

private:
    FileStatusWidget *m_fileStatusWidget;
    HistoryWidget *m_historyWidget;
    FileStates m_fileStates;

    Changesets parseChangeSets(QString changeSetsStr);
};

#endif // HGTABWIDGET_H
