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

#ifndef HGEXPWIDGET_H
#define HGEXPWIDGET_H

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

#define NUM_STAT_FILE_TYPES 7

class FileStatusWidget;


class HgExpWidget: public QTabWidget
{
    Q_OBJECT

public:
    HgExpWidget(QWidget *parent, QString remoteRepo, QString workFolderPath,
                unsigned char viewFileTypesBits = DEFAULT_HG_STAT_BITS);

    void updateWorkFolderFileList(QString fileList);
    void updateLocalRepoHgLogList(QString hgLogList);
    void setWorkFolderAndRepoNames(QString workFolderPath, QString remoteRepoPath);

    FileStates getFileStates() { return fileStates; }

    bool canCommit() const;
    bool canAdd() const;
    bool canRemove() const;
    bool canDoFolderDiff() const;

public slots:
    void clearSelections();

private:
    FileStatusWidget *fileStatusWidget;

    QWidget *historyGraphPageWidget;
    QWidget *historyGraphWidget;
    QWidget *historyGraphPanner;
    QWidget *historyPageWidget;

    FileStates fileStates;

    Changesets parseChangeSets(QString changeSetsStr);
};

#endif // HGEXPWIDGET_H
