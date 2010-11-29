/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    EasyMercurial

    Based on hgExplorer by Jari Korhonen
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

#include "hgtabwidget.h"
#include "hgrunner.h"
#include "common.h"
#include "changeset.h"
#include "hgaction.h"

#include <QMainWindow>
#include <QListWidget>
#include <QFileSystemWatcher>

QT_BEGIN_NAMESPACE
class QAction;
class QMenu;
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow();
    HgTabWidget *hgTabs;
    void writeSettings();

    //Paths to remote repo & workfolder
    //Local repo is directory "./hg/" under work folder
    QString remoteRepoPath;
    QString workFolderPath;
    QString currentBranch;
    Changesets currentHeads;
    Changesets currentParents;
    bool needNewLog;

protected:
    void closeEvent(QCloseEvent *event);

public slots:
    void hgRefresh();
    void commandCompleted(HgAction action, QString stdout);
    void commandFailed(HgAction action, QString stdout);
    void enableDisableActions();

private slots:
    void about();
    void settings();
    void open();
    void startupDialog();
    void clearSelections();

    void hgQueryPaths();
    void hgStat();
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

    void fsDirectoryChanged(QString);
    void fsFileChanged(QString);

private:
    void hgQueryBranch();
    void hgQueryHeads();
    void hgQueryParents();
    void hgLog();
    void hgLogIncremental();
    void createActions();
    void connectActions();
    void createMenus();
    void createToolBars();
    void createStatusBar();
    void readSettings();
    void splitChangeSets(QStringList *list, QString hgLogOutput);
    void presentLongStdoutToUser(QString stdo);

    QString listAllUpIpV4Addresses();
    QString filterTag(QString tag);

    QString getUserInfo() const;

    bool openLocal(QString);
    bool openRemote(QString, QString);
    bool openInit(QString);

    bool complainAboutFilePath(QString);
    bool complainAboutUnknownFolder(QString);
    bool complainAboutInitInRepo(QString);
    bool complainAboutInitFile(QString);
    bool complainAboutCloneToExisting(QString);
    bool complainAboutCloneToFile(QString);
    bool complainAboutCloneToExistingFolder(QString); //!!! not sure about this one

    bool askToInitExisting(QString);
    bool askToInitNew(QString);
    bool askToOpenParentRepo(QString, QString);
    bool askToOpenInsteadOfInit(QString);

    void showIncoming(QString);
    void showPullResult(QString);
    void showPushResult(QString);

    void clearState();

    void updateFileSystemWatcher();

    bool firstStart;

    //Actions enabled flags
    bool remoteRepoActionsEnabled;
    bool localRepoActionsEnabled;

    //File menu actions
    QAction *hgInitAct;
    QAction *hgCloneFromRemoteAct;
    QAction *openAct;
    QAction *settingsAct;
    QAction *exitAct;

    //Repo actions
    QAction *hgIncomingAct;
    QAction *hgPushAct;
    QAction *hgPullAct;
    QAction *hgRefreshAct;
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

    // Other actions
    QAction *clearSelectionsAct;

    QToolBar *fileToolBar;
    QToolBar *repoToolBar;
    QToolBar *workFolderToolBar;

    HgRunner *runner;

    void findDiffBinaryName();
    QString diffBinaryName;

    QFileSystemWatcher *fsWatcher;

    bool justMerged;
};

#endif
