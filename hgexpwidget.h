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
#include "statparser.h"

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
    void updateLocalRepoHeadsList(QString headList);
    void updateLocalRepoHgLogList(QString hgLogList);
    void updateLocalRepoParentsList(QString parentsList);
    void setWorkFolderAndRepoNames(QString workFolderPath, QString remoteRepoPath);
    QString getCurrentFileListLine();
    void getHistoryDiffRevisions(QString& revA, QString& revB);
    void getUpdateToRevRevision(QString& rev);
    void clearLists();
    void enableDisableOtherTabs(int tabPage);
    QString getStatFlags(void);
    unsigned char getFileTypesBits();


    QListWidget *workFolderFileList;
    QListWidget *localRepoHeadsList;
    QListWidget *localRepoHgLogList;

signals:
    void workFolderViewTypesChanged();

private slots:
    void copyComment();

private:
    FileStatusWidget *fileStatusWidget;

    QGroupBox   *grpRemoteRepo;
    QWidget     *workPageWidget;
    QWidget     *historyGraphPageWidget;
    QWidget     *historyGraphWidget;
    QWidget     *historyGraphPanner;
    QWidget     *historyPageWidget;
    QWidget     *headsPageWidget;

    QGroupBox   *grpLocalRepo;
    QVBoxLayout *mainLayout;
    QVBoxLayout *localRepoLayout;
    QVBoxLayout *parentsLayout;
    QListWidget *localRepoHgParentsList;
    QLabel      *parentsLabel;
    QMenu       *userListMenu;
    QAction     *copyCommentAct;

    QGroupBox   *grpWorkFolder;
    QHBoxLayout *workFolderLayout;
    QGroupBox   *grpViewFileTypes;
    QVBoxLayout *fileTypesLayout;
    QCheckBox   *chkViewFileTypes[NUM_STAT_FILE_TYPES];

    QVBoxLayout *historyLayout;

    QVBoxLayout *headsLayout;

    StatParser   statParser;

    QString     findRev(QString itemText, QString& smallRev);
    QStringList splitChangeSets(QString chgSetsStr);
    Changesets  parseChangeSets(QString changeSetsStr);

    int findLineStart(int nowIndex, QString chgSetsStr);
    void contextMenuEvent (QContextMenuEvent * event);
};

#endif // HGEXPWIDGET_H
