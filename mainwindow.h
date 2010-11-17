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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "hgexpwidget.h"
#include "hgrunner.h"
#include "common.h"

#include <QMainWindow>
#include <QListWidget>

QT_BEGIN_NAMESPACE
class QAction;
class QMenu;
QT_END_NAMESPACE

enum HGACTIONS
{
    ACT_NONE,
    ACT_STAT,
    ACT_HEADS,
    ACT_PARENTS,
    ACT_LOG,
    ACT_REMOVE,
    ACT_ADD,
    ACT_INCOMING,
    ACT_PUSH,
    ACT_PULL,
    ACT_CLONEFROMREMOTE,
    ACT_INIT,
    ACT_COMMIT,
    ACT_ANNOTATE,
    ACT_FILEDIFF,
    ACT_FOLDERDIFF,
    ACT_CHGSETDIFF,
    ACT_UPDATE,
    ACT_REVERT,
    ACT_MERGE,
    ACT_RESOLVE_LIST,
    ACT_SERVE,
    ACT_RESOLVE_MARK,
    ACT_RETRY_MERGE,
    ACT_TAG,
    ACT_HG_IGNORE,
};


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow();
    HgExpWidget *hgExp;
    void writeSettings();
    void enableDisableActions();

    //Paths to remote repo & workfolder
    //Local repo is directory "./hg/" under work folder
    QString remoteRepoPath;
    QString workFolderPath;

protected:
    void closeEvent(QCloseEvent *event);

public slots:
    void hgStat();
    void tabChanged(int currTab);
    void commandCompleted();
    void commandFailed();

private slots:
    void about();
    void settings();
    void startupDialog();

    void hgRemove();
    void hgAdd();
    void hgCommit();
    void hgFileDiff();
    void hgFolderDiff();
    void hgChgSetDiff();
    void hgUpdate();
    void hgRevert();
    void hgMerge();
    void hgRetryMerge();
    void hgCloneFromRemote();
    void hgInit();
    void hgIncoming();
    void hgPush();
    void hgPull();
    void hgUpdateToRev();
    void hgAnnotate();
    void hgResolveList();
    void hgResolveMark();
    void hgTag();
    void hgServe();
    void hgIgnore();

private:
    void hgHeads();
    void hgParents();
    void hgLog();
    void createActions();
    void connectActions();
    void createMenus();
    void createToolBars();
    void createStatusBar();
    void readSettings();
    void splitChangeSets(QStringList *list, QString hgLogOutput);
    int getCommentOrTag(QString& commentOrTag, QString question, QString dlgTitle);
    void presentLongStdoutToUser(QString stdo);
    void countModifications(QListWidget *workList, int& added, int& modified, int& removed, int& notTracked,
        int& selected,
        int& selectedAdded, int& selectedModified, int& selectedRemoved, int& selectedNotTracked);
    bool isSelectedModified(QListWidget *workList);
    bool areAllSelectedUntracked(QListWidget *workList);
    bool isSelectedDeletable(QListWidget *workList);
    bool areAllSelectedCommitable(QListWidget *workList);
    QString listAllUpIpV4Addresses();
    QString filterTag(QString tag);

    QString getUserInfo() const;

    bool firstStart;

    //Actions enabled flags
    bool remoteRepoActionsEnabled;
    bool localRepoActionsEnabled;

    //File menu actions
    QAction *hgInitAct;
    QAction *hgCloneFromRemoteAct;
    QAction *settingsAct;
    QAction *exitAct;

    //Repo actions
    QAction *hgIncomingAct;
    QAction *hgPushAct;
    QAction *hgPullAct;
    QAction *hgStatAct;
    QAction *hgFileDiffAct;
    QAction *hgFolderDiffAct;
    QAction *hgChgSetDiffAct;
    QAction *hgRevertAct;
    QAction *hgAddAct;
    QAction *hgRemoveAct;
    QAction *hgUpdateAct;
    QAction *hgCommitAct;
    QAction *hgMergeAct;
    QAction *hgRetryMergeAct;
    QAction *hgUpdateToRevAct;
    QAction *hgAnnotateAct;
    QAction *hgResolveListAct;
    QAction *hgResolveMarkAct;
    QAction *hgTagAct;
    QAction *hgIgnoreAct;
    QAction *hgServeAct;

    //Menus
    QMenu   *fileMenu;
    QMenu   *advancedMenu;
    QMenu   *helpMenu;

    //Help menu actions
    QAction *aboutAct;
    QAction *aboutQtAct;

    QToolBar *fileToolBar;
    QToolBar *repoToolBar;
    QToolBar *workFolderToolBar;

    HGACTIONS   runningAction;
    HgRunner    *runner;

    int             tabPage;
    unsigned char   initialFileTypesBits;
    bool            justMerged;
};

#endif
