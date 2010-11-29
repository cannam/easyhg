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

#include <QStringList>
#include <QDir>
#include <QNetworkInterface>
#include <QHostAddress>
#include <QHostInfo>
#include <QDesktopServices>
#include <QStatusBar>
#include <QMessageBox>
#include <QMenuBar>
#include <QApplication>
#include <QToolBar>
#include <QToolButton>
#include <QSettings>
#include <QInputDialog>

#include "mainwindow.h"
#include "multichoicedialog.h"
#include "startupdialog.h"
#include "colourset.h"
#include "debug.h"
#include "logparser.h"
#include "confirmcommentdialog.h"


MainWindow::MainWindow()
{
    QString wndTitle;

    fsWatcher = 0;

    createActions();
    createMenus();
    createToolBars();
    createStatusBar();

    runner = new HgRunner(this);
    connect(runner, SIGNAL(commandCompleted(HgAction, QString)),
            this, SLOT(commandCompleted(HgAction, QString)));
    connect(runner, SIGNAL(commandFailed(HgAction, QString)),
            this, SLOT(commandFailed(HgAction, QString)));
    statusBar()->addPermanentWidget(runner);

    setWindowTitle(tr("EasyMercurial"));

    remoteRepoPath = "";
    workFolderPath = "";

    readSettings();

    justMerged = false;
    hgTabs = new HgTabWidget((QWidget *) this, remoteRepoPath, workFolderPath);
    setCentralWidget(hgTabs);

    connect(hgTabs, SIGNAL(selectionChanged()),
            this, SLOT(enableDisableActions()));

    setUnifiedTitleAndToolBarOnMac(true);
    connectActions();
    clearState();
    enableDisableActions();

    if (firstStart) {
        startupDialog();
    }

    findDiffBinaryName();

    ColourSet *cs = ColourSet::instance();
    cs->clearDefaultNames();
    cs->addDefaultName("");
    cs->addDefaultName(getUserInfo());

    if (workFolderPath == "") {
        open();
    }

    hgQueryPaths();
}


void MainWindow::closeEvent(QCloseEvent *)
{
    writeSettings();
    delete fsWatcher;
}


QString MainWindow::getUserInfo() const
{
    QSettings settings;
    settings.beginGroup("User Information");
    QString name = settings.value("name", getUserRealName()).toString();
    QString email = settings.value("email", "").toString();

    QString identifier;

    if (email != "") {
	identifier = QString("%1 <%2>").arg(name).arg(email);
    } else {
	identifier = name;
    }

    return identifier;
}

void MainWindow::about()
{
   QMessageBox::about(this, tr("About EasyMercurial"),
                      tr("<qt><h2>About EasyMercurial</h2>"
                         "<p>EasyMercurial is a simple user interface for the "
                         "Mercurial version control system.</p>"
                         "<p>EasyMercurial is based on hgExplorer by "
                         "Jari Korhonen, with thanks.<br>EasyMercurial development carried out by "
                         "Chris Cannam for soundsoftware.ac.uk at the Centre for Digital Music, Queen Mary, University of London."
                         "<ul><li>Copyright &copy; 2010 Jari Korhonen</li>"
                         "<li>Copyright &copy; 2010 Chris Cannam</li>"
                         "<li>Copyright &copy; 2010 Queen Mary, University of London</li>"
                         "</ul>"
                         "<p> This program is free software; you can redistribute it and/or "
                         "modify it under the terms of the GNU General Public License as "
                         "published by the Free Software Foundation; either version 2 of the "
                         "License, or (at your option) any later version.  See the file "
                         "COPYING included with this distribution for more information."));
}

void MainWindow::clearSelections()
{
    hgTabs->clearSelections();
}

void MainWindow::hgRefresh()
{
    clearState();
    hgQueryPaths();
}

void MainWindow::hgStat()
{
    QStringList params;
    params << "stat" << "-ardum";
    runner->requestAction(HgAction(ACT_STAT, workFolderPath, params));
}

void MainWindow::hgQueryPaths()
{
    QStringList params;
    params << "paths";
    runner->requestAction(HgAction(ACT_QUERY_PATHS, workFolderPath, params));
}

void MainWindow::hgQueryBranch()
{
    QStringList params;
    params << "branch";
    runner->requestAction(HgAction(ACT_QUERY_BRANCH, workFolderPath, params));
}

void MainWindow::hgQueryHeads()
{
    QStringList params;
    params << "heads";
    // on empty repos, "hg heads" will fail -- we don't care about that.
    runner->requestAction(HgAction(ACT_QUERY_HEADS, workFolderPath, params));
}

void MainWindow::hgLog()
{
    QStringList params;
    params << "log";
    params << "--template";
    params << "id: {rev}:{node|short}\\nauthor: {author}\\nbranch: {branches}\\ntag: {tag}\\ndatetime: {date|isodate}\\ntimestamp: {date|hgdate}\\nage: {date|age}\\nparents: {parents}\\ncomment: {desc|json}\\n\\n";
    
    runner->requestAction(HgAction(ACT_LOG, workFolderPath, params));
}

void MainWindow::hgLogIncremental()
{
    QStringList params;
    params << "log";

    foreach (Changeset *head, currentHeads) {
        int n = head->number();
        params << "--prune" << QString("%1").arg(n);
    }
        
    params << "--template";
    params << "id: {rev}:{node|short}\\nauthor: {author}\\nbranch: {branches}\\ntag: {tag}\\ndatetime: {date|isodate}\\ntimestamp: {date|hgdate}\\nage: {date|age}\\nparents: {parents}\\ncomment: {desc|json}\\n\\n";
    
    runner->requestAction(HgAction(ACT_LOG_INCREMENTAL, workFolderPath, params));
}

void MainWindow::hgQueryParents()
{
    QStringList params;
    params << "parents";
    runner->requestAction(HgAction(ACT_QUERY_PARENTS, workFolderPath, params));
}

void MainWindow::hgAnnotate()
{
    QStringList params;
    QString currentFile;//!!! = hgTabs -> getCurrentFileListLine();
    
    if (!currentFile.isEmpty())
    {
        params << "annotate" << "--" << currentFile.mid(2);   //Jump over status marker characters (e.g "M ")

        runner->requestAction(HgAction(ACT_ANNOTATE, workFolderPath, params));
    }
}

void MainWindow::hgResolveMark()
{
    QStringList params;
    QString currentFile;//!!! = hgTabs -> getCurrentFileListLine();

    if (!currentFile.isEmpty())
    {
        params << "resolve" << "--mark" << "--" << currentFile.mid(2);   //Jump over status marker characters (e.g "M ")

        runner->requestAction(HgAction(ACT_RESOLVE_MARK, workFolderPath, params));
    }
}

void MainWindow::hgResolveList()
{
    QStringList params;

    params << "resolve" << "--list";
    runner->requestAction(HgAction(ACT_RESOLVE_LIST, workFolderPath, params));
}

void MainWindow::hgAdd()
{
    QStringList params;

    // hgExplorer permitted adding "all" files -- I'm not sure
    // that one is a good idea, let's require the user to select

    QStringList files = hgTabs->getSelectedAddableFiles();

    if (!files.empty()) {
        params << "add" << "--" << files;
        runner->requestAction(HgAction(ACT_ADD, workFolderPath, params));
    }
}


void MainWindow::hgRemove()
{
    QStringList params;

    QStringList files = hgTabs->getSelectedRemovableFiles();

    //!!! todo: confirmation dialog (with file list in it) (or do we
    // need that? all it does is move the files to the removed
    // list... doesn't it?)

    if (!files.empty()) {
        params << "remove" << "--after" << "--force" << "--" << files;
        runner->requestAction(HgAction(ACT_REMOVE, workFolderPath, params));
    }

/*!!!
        QString currentFile;//!!! = hgTabs -> getCurrentFileListLine();

        if (!currentFile.isEmpty())
        {
            if (QMessageBox::Ok == QMessageBox::warning(this, "Remove file", "Really remove file " + currentFile.mid(2) + "?",
                QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Cancel))
            {
                params << "remove" << "--after" << "--force" << "--" << currentFile.mid(2);   //Jump over status marker characters (e.g "M ")

                runner -> startHgCommand(workFolderPath, params);
                runningAction = ACT_REMOVE;
            }
        }
        */
}

void MainWindow::hgCommit()
{
    QStringList params;
    QString comment;

    QStringList files = hgTabs->getSelectedCommittableFiles();
    if (files.empty()) files = hgTabs->getAllCommittableFiles();

    if (ConfirmCommentDialog::confirmAndGetLongComment
        (this,
         tr("Commit files"),
         tr("<h3>Commit files</h3><p>You are about to commit the following files:"),
         tr("<h3>Commit files</h3><p>You are about to commit %1 files."),
         files,
         comment)) {

        if ((justMerged == false) && //!!! review usage of justMerged, and how it interacts with asynchronous request queue
            !files.empty()) {
            // User wants to commit selected file(s) (and this is not merge commit, which would fail if we selected files)
            params << "commit" << "--message" << comment << "--user" << getUserInfo() << "--" << files;
        } else {
            // Commit all changes
            params << "commit" << "--message" << comment << "--user" << getUserInfo();
        }
        
        runner->requestAction(HgAction(ACT_COMMIT, workFolderPath, params));
    }
}

QString MainWindow::filterTag(QString tag)
{
    for(int i = 0; i < tag.size(); i++)
    {
        if (tag[i].isLower() || tag[i].isUpper() || tag[i].isDigit() || (tag[i] == QChar('.')))
        {
            //ok
        }
        else
        {
            tag[i] = QChar('_');
        }
    }
    return tag;
}


void MainWindow::hgTag()
{
    QStringList params;
    QString tag;

    if (ConfirmCommentDialog::confirmAndGetShortComment
        (this,
         tr("Tag"),
         tr("Enter tag:"),
         tag)) {
        if (!tag.isEmpty()) //!!! do something better if it is empty
        {
            params << "tag" << "--user" << getUserInfo() << filterTag(tag);
            
            runner->requestAction(HgAction(ACT_TAG, workFolderPath, params));
        }
    }
}


void MainWindow::hgIgnore()
{
    QString hgIgnorePath;
    QStringList params;
    QString editorName;

    hgIgnorePath = workFolderPath;
    hgIgnorePath += ".hgignore";
    
    params << hgIgnorePath;

//!!!    
#ifdef Q_OS_LINUX

        editorName = "gedit";

#else

        editorName = """C:\\Program Files\\Windows NT\\Accessories\\wordpad.exe""";

#endif

    HgAction action(ACT_HG_IGNORE, workFolderPath, params);
    action.executable = editorName;

    runner->requestAction(action);
}

void MainWindow::findDiffBinaryName()
{
    QSettings settings;
    QString diff = settings.value("extdiffbinary", "").toString();
    if (diff == "") {
        QStringList bases;
        bases << "opendiff" << "kompare" << "kdiff3" << "meld";
        bool found = false;
        foreach (QString base, bases) {
            diff = findExecutable(base);
            if (diff != base) {
                found = true;
                break;
            }
        }
        if (found) {
            settings.setValue("extdiffbinary", diff);
        } else {
            diff = "";
        }
    }
    diffBinaryName = diff;
}

void MainWindow::hgFileDiff()
{
        QStringList params;
/*!!!
        QString currentFile = hgTabs -> getCurrentFileListLine();

        if (!currentFile.isEmpty())
        {
            //Diff parent file against working folder file
            params << "kdiff3" << "--" << currentFile.mid(2);
            runner -> startHgCommand(workFolderPath, params);
            runningAction = ACT_FILEDIFF;
        }
    */
}


void MainWindow::hgFolderDiff()
{
    if (diffBinaryName == "") return;

    QStringList params;

    // Diff parent against working folder (folder diff)

    params << "--config" << "extensions.extdiff=" << "extdiff" << "-p";
    params << diffBinaryName;

    runner->requestAction(HgAction(ACT_FOLDERDIFF, workFolderPath, params));
}


void MainWindow::hgChgSetDiff()
{
        QStringList params;

        //Diff 2 history log versions against each other
        QString revA;
        QString revB;
/*!!!
        hgTabs -> getHistoryDiffRevisions(revA, revB);

        if ((!revA.isEmpty()) && (!revB.isEmpty()))
        {
            params << "kdiff3" << "--rev" << revA << "--rev" << revB;
            runner -> startHgCommand(workFolderPath, params);
            runningAction = ACT_CHGSETDIFF;
        }
        else
        {
            QMessageBox::information(this, tr("Changeset diff"), tr("Please select two changesets from history list or heads list first."));
        }
        */
}



void MainWindow::hgUpdate()
{
    QStringList params;

    params << "update";
    
    runner->requestAction(HgAction(ACT_UPDATE, workFolderPath, params));
}


void MainWindow::hgUpdateToRev()
{
        QStringList params;
        QString rev;
/*!!!
        hgTabs -> getUpdateToRevRevision(rev);

        hgTabs -> setCurrentIndex(WORKTAB);
        enableDisableActions();

        params << "update" << "--rev" << rev << "--clean";

        runner -> startHgCommand(workFolderPath, params);

        runningAction = ACT_UPDATE;
        */

}


void MainWindow::hgRevert()
{
    QStringList params;
    QString comment;

    QStringList files = hgTabs->getSelectedRevertableFiles();
    if (files.empty()) files = hgTabs->getAllRevertableFiles();
    
    if (ConfirmCommentDialog::confirmDangerousFilesAction
        (this,
         tr("Revert files"),
         tr("<h3>Revert files</h3><p>You are about to <b>revert</b> the following files to their previous committed state.<br><br>This will <b>throw away any changes</b> that you have made to these files but have not committed."),
         tr("<h3>Revert files</h3><p>You are about to <b>revert</b> %1 files.<br><br>This will <b>throw away any changes</b> that you have made to these files but have not committed."),
         files)) {
        
        if (files.empty()) {
            params << "revert" << "--no-backup";
        } else {
            params << "revert" << "--no-backup" << "--" << files;
        }
        
        runner->requestAction(HgAction(ACT_REVERT, workFolderPath, params));
    }
}

void MainWindow::hgRetryMerge()
{
    QStringList params;

    params << "resolve" << "--all";
    runner->requestAction(HgAction(ACT_RETRY_MERGE, workFolderPath, params));
}


void MainWindow::hgMerge()
{
    QStringList params;

    params << "merge";
    
    runner->requestAction(HgAction(ACT_MERGE, workFolderPath, params));
}


void MainWindow::hgCloneFromRemote()
{
    QStringList params;

    if (!QDir(workFolderPath).exists()) {
        if (!QDir().mkpath(workFolderPath)) {
            DEBUG << "hgCloneFromRemote: Failed to create target path "
                  << workFolderPath << endl;
            //!!! report error
            return;
        }
    }

    params << "clone" << remoteRepoPath << workFolderPath;
    
    hgTabs->setWorkFolderAndRepoNames(workFolderPath, remoteRepoPath);
    hgTabs->updateWorkFolderFileList("");

    runner->requestAction(HgAction(ACT_CLONEFROMREMOTE, workFolderPath, params));
}

void MainWindow::hgInit()
{
    QStringList params;

    params << "init";
    params << workFolderPath;

    runner->requestAction(HgAction(ACT_INIT, workFolderPath, params));
}

void MainWindow::hgIncoming()
{
    QStringList params;

    params << "incoming" << "--newest-first" << remoteRepoPath;

    runner->requestAction(HgAction(ACT_INCOMING, workFolderPath, params));
}


void MainWindow::hgPull()
{
    QStringList params;

    params << "pull" << remoteRepoPath;

    runner->requestAction(HgAction(ACT_PULL, workFolderPath, params));
}


void MainWindow::hgPush()
{
    QStringList params;

    params << "push" << remoteRepoPath;

    runner->requestAction(HgAction(ACT_PUSH, workFolderPath, params));
}

QString MainWindow::listAllUpIpV4Addresses()
{
    QString ret;
    QList<QNetworkInterface> ifaces = QNetworkInterface::allInterfaces();

    for (int i = 0; i < ifaces.count(); i++)
    {
        QNetworkInterface iface = ifaces.at(i);

        if (iface.flags().testFlag(QNetworkInterface::IsUp) && !iface.flags().testFlag(QNetworkInterface::IsLoopBack))
        {
            for (int j=0; j<iface.addressEntries().count(); j++)
            {
                QHostAddress tmp = iface.addressEntries().at(j).ip();
                if (QAbstractSocket::IPv4Protocol == tmp.protocol())
                {
                    if (!ret.isEmpty())
                    {
                        ret += " ";
                    }
                    ret += tmp.toString();
                }
            }
        }
    }
    return ret;
}

void MainWindow::clearState()
{
    foreach (Changeset *cs, currentParents) delete cs;
    currentParents.clear();
    foreach (Changeset *cs, currentHeads) delete cs;
    currentHeads.clear();
    currentBranch = "";
    needNewLog = true;
}

void MainWindow::hgServe()
{
    QStringList params;
    QString msg;

    QString addrs = listAllUpIpV4Addresses();
    QTextStream(&msg) << "Server running on address(es) (" << addrs << "), port 8000";
    params << "serve";

    runner->requestAction(HgAction(ACT_SERVE, workFolderPath, params));
    
    QMessageBox::information(this, "Serve", msg, QMessageBox::Close);
//!!!    runner -> killCurrentCommand();
}

void MainWindow::startupDialog()
{
    StartupDialog *dlg = new StartupDialog(this);
    if (dlg->exec()) firstStart = false;
}

void MainWindow::open()
{
    bool done = false;

    while (!done) {

        MultiChoiceDialog *d = new MultiChoiceDialog
                               (tr("Open Repository"),
                                tr("<qt><big>What would you like to open?</big></qt>"),
                                this);

        d->addChoice("remote",
                     tr("<qt><center><img src=\":images/browser-64.png\"><br>Remote repository</center></qt>"),
                     tr("Open a remote Mercurial repository, by cloning from its URL into a local folder."),
                     MultiChoiceDialog::UrlToDirectoryArg);

        d->addChoice("local",
                     tr("<qt><center><img src=\":images/hglogo-64.png\"><br>Local repository</center></qt>"),
                     tr("Open an existing local Mercurial repository."),
                     MultiChoiceDialog::DirectoryArg);

        d->addChoice("init",
                     tr("<qt><center><img src=\":images/hdd_unmount-64.png\"><br>File folder</center></qt>"),
                     tr("Open a local folder, by creating a Mercurial repository in it."),
                     MultiChoiceDialog::DirectoryArg);

        d->setCurrentChoice("local");

        if (d->exec() == QDialog::Accepted) {

            QString choice = d->getCurrentChoice();
            QString arg = d->getArgument().trimmed();

            bool result = false;

            if (choice == "local") {
                result = openLocal(arg);
            } else if (choice == "remote") {
                result = openRemote(arg, d->getAdditionalArgument().trimmed());
            } else if (choice == "init") {
                result = openInit(arg);
            }

            if (result) {
                enableDisableActions();
                clearState();
                hgQueryPaths();
                done = true;
            }

        } else {

            // cancelled
            done = true;
        }

        delete d;
    }
}

bool MainWindow::complainAboutFilePath(QString arg)
{    
    QMessageBox::critical
        (this, tr("File chosen"),
         tr("<qt><b>Folder required</b><br><br>You asked to open \"%1\".<br>This is a file; to open a repository, you need to choose a folder.</qt>").arg(xmlEncode(arg)));
    return false;
}

bool MainWindow::complainAboutUnknownFolder(QString arg)
{    
    QMessageBox::critical
        (this, tr("Folder does not exist"),
         tr("<qt><b>Folder does not exist</b><br><br>You asked to open \"%1\".<br>This folder does not exist, and it cannot be created because its parent does not exist either.</qt>").arg(xmlEncode(arg)));
    return false;
}

bool MainWindow::complainAboutInitInRepo(QString arg)
{
    QMessageBox::critical
        (this, tr("Path is in existing repository"),
         tr("<qt><b>Path is in an existing repository</b><br><br>You asked to initialise a repository at \"%1\".<br>This path is already inside an existing repository.</qt>").arg(xmlEncode(arg)));
    return false;
}

bool MainWindow::complainAboutInitFile(QString arg)
{
    QMessageBox::critical
        (this, tr("Path is a file"),
         tr("<qt><b>Path is a file</b><br><br>You asked to initialise a repository at \"%1\".<br>This is an existing file; it is only possible to initialise in folders.</qt>").arg(xmlEncode(arg)));
    return false;
}

bool MainWindow::complainAboutCloneToExisting(QString arg)
{
    QMessageBox::critical
        (this, tr("Path is in existing repository"),
         tr("<qt><b>Local path is in an existing repository</b><br><br>You asked to open a remote repository by cloning it to the local path \"%1\".<br>This path is already inside an existing repository.<br>Please provide a new folder name for the local repository.</qt>").arg(xmlEncode(arg)));
    return false;
}

bool MainWindow::complainAboutCloneToFile(QString arg)
{
    QMessageBox::critical
        (this, tr("Path is a file"),
         tr("<qt><b>Local path is a file</b><br><br>You asked to open a remote repository by cloning it to the local path \"%1\".<br>This path is an existing file.<br>Please provide a new folder name for the local repository.</qt>").arg(xmlEncode(arg)));
    return false;
}

bool MainWindow::complainAboutCloneToExistingFolder(QString arg)
{
    QMessageBox::critical
        (this, tr("Folder exists"),
         tr("<qt><b>Local folder already exists</b><br><br>You asked to open a remote repository by cloning it to the local path \"%1\".<br>This is the path of an existing folder.<br>Please provide a new folder name for the local repository.</qt>").arg(xmlEncode(arg)));
    return false;
}

bool MainWindow::askToOpenParentRepo(QString arg, QString parent)
{
    return (QMessageBox::question
            (this, tr("Path is inside a repository"),
             tr("<qt><b>Open the repository that contains this path?</b><br><br>You asked to open \"%1\".<br>This is not the root folder of a repository.<br>But it is inside a repository, whose root is at \"%2\". <br><br>Would you like to open that repository instead?</qt>")
             .arg(xmlEncode(arg)).arg(xmlEncode(parent)),
             QMessageBox::Ok | QMessageBox::Cancel,
             QMessageBox::Ok)
            == QMessageBox::Ok);
}

bool MainWindow::askToInitExisting(QString arg)
{
    return (QMessageBox::question
            (this, tr("Folder has no repository"),
             tr("<qt><b>Initialise a repository here?</b><br><br>You asked to open \"%1\".<br>This folder does not contain a Mercurial repository.<br><br>Would you like to initialise a repository here?</qt>")
             .arg(xmlEncode(arg)),
             QMessageBox::Ok | QMessageBox::Cancel,
             QMessageBox::Ok)
            == QMessageBox::Ok);
}

bool MainWindow::askToInitNew(QString arg)
{
    return (QMessageBox::question
            (this, tr("Folder does not exist"),
             tr("<qt><b>Initialise a new repository?</b><br><br>You asked to open \"%1\".<br>This folder does not yet exist.<br><br>Would you like to create the folder and initialise a new empty repository in it?</qt>")
             .arg(xmlEncode(arg)),
             QMessageBox::Ok | QMessageBox::Cancel,
             QMessageBox::Ok)
            == QMessageBox::Ok);
}

bool MainWindow::askToOpenInsteadOfInit(QString arg)
{
    return (QMessageBox::question
            (this, tr("Repository exists"),
             tr("<qt><b>Open existing repository?</b><br><br>You asked to initialise a new repository at \"%1\".<br>This folder already contains a repository.  Would you like to open it?</qt>")
             .arg(xmlEncode(arg)),
             QMessageBox::Ok | QMessageBox::Cancel,
             QMessageBox::Ok)
            == QMessageBox::Ok);
}

bool MainWindow::openLocal(QString local)
{
    DEBUG << "open " << local << endl;

    FolderStatus status = getFolderStatus(local);
    QString containing = getContainingRepoFolder(local);

    switch (status) {

    case FolderHasRepo:
        // fine
        break;

    case FolderExists:
        if (containing != "") {
            if (!askToOpenParentRepo(local, containing)) return false;
            local = containing;
        } else {
            //!!! No -- this is likely to happen far more by accident
            // than because the user actually wanted to init something.
            // Don't ask, just politely reject.
            if (!askToInitExisting(local)) return false;
            return openInit(local);
        }
        break;

    case FolderParentExists:
        if (containing != "") {
            if (!askToOpenParentRepo(local, containing)) return false;
            local = containing;
        } else {
            if (!askToInitNew(local)) return false;
            return openInit(local);
        }
        break;

    case FolderUnknown:
        if (containing != "") {
            if (!askToOpenParentRepo(local, containing)) return false;
            local = containing;
        } else {
            return complainAboutUnknownFolder(local);
        }
        break;
        
    case FolderIsFile:
        return complainAboutFilePath(local);
    }

    workFolderPath = local;
    remoteRepoPath = "";
    return true;
}    

bool MainWindow::openRemote(QString remote, QString local)
{
    DEBUG << "clone " << remote << " to " << local << endl;

    FolderStatus status = getFolderStatus(local);
    QString containing = getContainingRepoFolder(local);

    DEBUG << "status = " << status << ", containing = " << containing << endl;

    if (status == FolderHasRepo || containing != "") {
        return complainAboutCloneToExisting(local);
    }

    if (status == FolderIsFile) {
        return complainAboutCloneToFile(local);
    }

    if (status == FolderUnknown) {
        return complainAboutUnknownFolder(local);
    }

    if (status == FolderExists) {
        //!!! we can do better than this surely?
        return complainAboutCloneToExistingFolder(local);
    }

    workFolderPath = local;
    remoteRepoPath = remote;
    hgCloneFromRemote();

    return true;
}

bool MainWindow::openInit(QString local)
{
    DEBUG << "openInit " << local << endl;

    FolderStatus status = getFolderStatus(local);
    QString containing = getContainingRepoFolder(local);

    DEBUG << "status = " << status << ", containing = " << containing << endl;

    if (status == FolderHasRepo) {
        if (!askToOpenInsteadOfInit(local)) return false;
    }

    if (containing != "") {
        return complainAboutInitInRepo(local);
    }

    if (status == FolderIsFile) {
        return complainAboutInitFile(local);
    }

    if (status == FolderUnknown) {
        return complainAboutUnknownFolder(local);
    }

    workFolderPath = local;
    remoteRepoPath = "";
    hgInit();
    return true;
}

void MainWindow::settings()
{
/*!!!
    SettingsDialog *settingsDlg = new SettingsDialog(this);
    settingsDlg->setModal(true);
    settingsDlg->exec();
    hgTabs -> clearLists();
    enableDisableActions();
    hgStat();
*/
}

#define STDOUT_NEEDS_BIG_WINDOW 512
#define SMALL_WND_W     500
#define SMALL_WND_H     300

#define BIG_WND_W       1024
#define BIG_WND_H       768


void MainWindow::presentLongStdoutToUser(QString stdo)
{
    if (!stdo.isEmpty())
    {
        QDialog dlg;

        if (stdo.length() > STDOUT_NEEDS_BIG_WINDOW)
        {
            dlg.setMinimumWidth(BIG_WND_W);
            dlg.setMinimumHeight(BIG_WND_H);
        }
        else
        {
            dlg.setMinimumWidth(SMALL_WND_W);
            dlg.setMinimumHeight(SMALL_WND_H);
        }

        QVBoxLayout *box = new QVBoxLayout;
        QListWidget *list = new QListWidget;
        list-> addItems(stdo.split("\n"));
        QPushButton *btn = new QPushButton(tr("Ok"));
        connect(btn, SIGNAL(clicked()), &dlg, SLOT(accept()));

        box -> addWidget(list);
        box -> addWidget(btn);
        dlg.setLayout(box);

        dlg.exec();
    }
    else
    {
        QMessageBox::information(this, tr("EasyMercurial"), tr("Mercurial command did not return any output."));
    }
}

void MainWindow::updateFileSystemWatcher()
{
    //!!! this needs to be incremental when something changes

    delete fsWatcher;
    fsWatcher = new QFileSystemWatcher();
    std::deque<QString> pending;
    pending.push_back(workFolderPath);
    while (!pending.empty()) {
        QString path = pending.front();
        pending.pop_front();
        fsWatcher->addPath(path);
        DEBUG << "Added to file system watcher: " << path << endl;
        QDir d(path);
        if (d.exists()) {
            d.setFilter(QDir::Dirs | QDir::NoDotAndDotDot | QDir::Readable);
            foreach (QString entry, d.entryList()) {
                if (entry == ".hg") continue;
                QString entryPath = d.absoluteFilePath(entry);
                pending.push_back(entryPath);
            }
        }
    }
    connect(fsWatcher, SIGNAL(directoryChanged(QString)),
            this, SLOT(fsDirectoryChanged(QString)));
    connect(fsWatcher, SIGNAL(fileChanged(QString)),
            this, SLOT(fsFileChanged(QString)));
}

void MainWindow::fsDirectoryChanged(QString)
{
    hgStat();
}

void MainWindow::fsFileChanged(QString)
{
    hgStat();
}

void MainWindow::showIncoming(QString output)
{
    runner->hide();
    QMessageBox::information(this, "Incoming", output);
}

void MainWindow::showPushResult(QString output)
{
    runner->hide();
    QMessageBox::information(this, "Push", output);
}

void MainWindow::showPullResult(QString output)
{
    runner->hide();
    QMessageBox::information(this, "Pull", output);
}

void MainWindow::commandFailed(HgAction action, QString stderr)
{
    DEBUG << "MainWindow::commandFailed" << endl;

    // Some commands we just have to ignore bad return values from:

    switch(action.action) {
    case ACT_NONE:
        // uh huh
        return;
    case ACT_INCOMING:
        // returns non-zero code if the check was successful but there
        // are no changes pending
        return;
    case ACT_FOLDERDIFF:
    case ACT_FILEDIFF:
    case ACT_CHGSETDIFF:
        // external program, unlikely to be anything useful in stderr
        // and some return with failure codes when something as basic
        // as the user closing the window via the wm happens
        return;

    default:
        break;
    }

    QString command = action.executable;
    if (command == "") command = "hg";
    foreach (QString arg, action.params) {
        command += " " + arg;
    }

    QString message = tr("<qt><h3>Command failed</h3>"
                         "<p>The following command failed:</p>"
                         "<code>%1</code>"
                         "%2</qt>")
        .arg(command)
        .arg((stderr.trimmed() != "") ?
             tr("<p>Its output said:</p><code>%1</code>")
             .arg(xmlEncode(stderr.left(800))
                  .replace("\n", "<br>"))
             : "");

    QMessageBox::warning(this, tr("Command failed"), message);
}

void MainWindow::commandCompleted(HgAction completedAction, QString output)
{
    bool shouldHgStat = false;

    HGACTIONS action = completedAction.action;

    if (action == ACT_NONE) return;

    switch(action) {

    case ACT_QUERY_PATHS:
    {
        DEBUG << "stdout is " << output << endl;
        LogParser lp(output, "=");
        LogList ll = lp.parse();
        DEBUG << ll.size() << " results" << endl;
        if (!ll.empty()) {
            remoteRepoPath = lp.parse()[0]["default"].trimmed();
            DEBUG << "Set remote path to " << remoteRepoPath << endl;
        }
        MultiChoiceDialog::addRecentArgument("local", workFolderPath);
        MultiChoiceDialog::addRecentArgument("remote", remoteRepoPath);
        hgTabs->setWorkFolderAndRepoNames(workFolderPath, remoteRepoPath);
        break;
    }

    case ACT_QUERY_BRANCH:
        currentBranch = output.trimmed();
        break;

    case ACT_STAT:
        hgTabs->updateWorkFolderFileList(output);
        updateFileSystemWatcher();
        break;
        
    case ACT_INCOMING:
        showIncoming(output);
        break;

    case ACT_ANNOTATE:
    case ACT_RESOLVE_LIST:
    case ACT_RESOLVE_MARK:
        presentLongStdoutToUser(output);
        shouldHgStat = true;
        break;
        
    case ACT_PULL:
        showPullResult(output);
        shouldHgStat = true;
        break;
        
    case ACT_PUSH:
        showPushResult(output);
        break;
        
    case ACT_INIT:
        MultiChoiceDialog::addRecentArgument("init", workFolderPath);
        MultiChoiceDialog::addRecentArgument("local", workFolderPath);
        enableDisableActions();
        shouldHgStat = true;
        break;
        
    case ACT_CLONEFROMREMOTE:
        MultiChoiceDialog::addRecentArgument("local", workFolderPath);
        MultiChoiceDialog::addRecentArgument("remote", remoteRepoPath);
        MultiChoiceDialog::addRecentArgument("remote", workFolderPath, true);
        QMessageBox::information(this, "Clone", output);
        enableDisableActions();
        shouldHgStat = true;
        break;
        
    case ACT_LOG:
        hgTabs->setNewLog(output);
        needNewLog = false;
        break;
        
    case ACT_LOG_INCREMENTAL:
        hgTabs->addIncrementalLog(output);
        break;
        
    case ACT_QUERY_PARENTS:
        foreach (Changeset *cs, currentParents) delete cs;
        currentParents = Changeset::parseChangesets(output);
        break;
        
    case ACT_QUERY_HEADS:
        foreach (Changeset *cs, currentHeads) delete cs;
        currentHeads = Changeset::parseChangesets(output);
        break;
        
    case ACT_REMOVE:
    case ACT_ADD:
    case ACT_COMMIT:
    case ACT_REVERT:
        hgTabs->clearSelections();
        shouldHgStat = true;
        break;

    case ACT_FILEDIFF:
    case ACT_FOLDERDIFF:
    case ACT_CHGSETDIFF:
    case ACT_SERVE:
    case ACT_TAG:
    case ACT_HG_IGNORE:
        shouldHgStat = true;
        break;
        
    case ACT_UPDATE:
        QMessageBox::information(this, tr("Update"), output);
        shouldHgStat = true;
        break;
        
    case ACT_MERGE:
        QMessageBox::information(this, tr("Merge"), output);
        shouldHgStat = true;
        justMerged = true;
        break;
        
    case ACT_RETRY_MERGE:
        QMessageBox::information(this, tr("Merge retry"), tr("Merge retry successful."));
        shouldHgStat = true;
        justMerged = true;
        break;
        
    default:
        break;
    }

    enableDisableActions();

    // Typical sequence goes paths -> branch -> stat -> heads -> parents -> log
    if (action == ACT_QUERY_PATHS) {
        hgQueryBranch();
    } else if (action == ACT_QUERY_BRANCH) {
        hgStat();
    } else if (action == ACT_STAT) {
        hgQueryHeads();
    } else if (action == ACT_QUERY_HEADS) {
        hgQueryParents();
    } else if (action == ACT_QUERY_PARENTS) {
        if (needNewLog) {
            hgLog();
        } else {
            hgLogIncremental();
        }
    } else 
/* Move to commandFailed
if ((runningAction == ACT_MERGE) && (exitCode != 0))
            {
                // If we had a failed merge, offer to retry
                if (QMessageBox::Ok == QMessageBox::information(this, tr("Retry merge ?"), tr("Merge attempt failed. retry ?"), QMessageBox::Ok | QMessageBox::Cancel))
                {
                    runningAction = ACT_NONE;
                    hgRetryMerge();
                }
                else
                {
                    runningAction = ACT_NONE;
                    hgStat();
                }
            }
            else
            {
*/
        if (shouldHgStat) {
            hgQueryPaths();
        }
}

void MainWindow::connectActions()
{
    connect(exitAct, SIGNAL(triggered()), this, SLOT(close()));
    connect(aboutAct, SIGNAL(triggered()), this, SLOT(about()));
    connect(aboutQtAct, SIGNAL(triggered()), qApp, SLOT(aboutQt()));

    connect(hgRefreshAct, SIGNAL(triggered()), this, SLOT(hgRefresh()));
    connect(hgRemoveAct, SIGNAL(triggered()), this, SLOT(hgRemove()));
    connect(hgAddAct, SIGNAL(triggered()), this, SLOT(hgAdd()));
    connect(hgCommitAct, SIGNAL(triggered()), this, SLOT(hgCommit()));
    connect(hgFileDiffAct, SIGNAL(triggered()), this, SLOT(hgFileDiff()));
    connect(hgFolderDiffAct, SIGNAL(triggered()), this, SLOT(hgFolderDiff()));
    connect(hgChgSetDiffAct, SIGNAL(triggered()), this, SLOT(hgChgSetDiff()));
    connect(hgUpdateAct, SIGNAL(triggered()), this, SLOT(hgUpdate()));
    connect(hgRevertAct, SIGNAL(triggered()), this, SLOT(hgRevert()));
    connect(hgMergeAct, SIGNAL(triggered()), this, SLOT(hgMerge()));
    connect(hgRetryMergeAct, SIGNAL(triggered()), this, SLOT(hgRetryMerge()));
    connect(hgTagAct, SIGNAL(triggered()), this, SLOT(hgTag()));
    connect(hgIgnoreAct, SIGNAL(triggered()), this, SLOT(hgIgnore()));

    connect(settingsAct, SIGNAL(triggered()), this, SLOT(settings()));
    connect(openAct, SIGNAL(triggered()), this, SLOT(open()));

    connect(hgInitAct, SIGNAL(triggered()), this, SLOT(hgInit()));
    connect(hgCloneFromRemoteAct, SIGNAL(triggered()), this, SLOT(hgCloneFromRemote()));
    connect(hgIncomingAct, SIGNAL(triggered()), this, SLOT(hgIncoming()));
    connect(hgPullAct, SIGNAL(triggered()), this, SLOT(hgPull()));
    connect(hgPushAct, SIGNAL(triggered()), this, SLOT(hgPush()));

//    connect(hgTabs, SIGNAL(currentChanged(int)), this, SLOT(tabChanged(int)));

    connect(hgUpdateToRevAct, SIGNAL(triggered()), this, SLOT(hgUpdateToRev()));
    connect(hgAnnotateAct, SIGNAL(triggered()), this, SLOT(hgAnnotate()));
    connect(hgResolveListAct, SIGNAL(triggered()), this, SLOT(hgResolveList()));
    connect(hgResolveMarkAct, SIGNAL(triggered()), this, SLOT(hgResolveMark()));
    connect(hgServeAct, SIGNAL(triggered()), this, SLOT(hgServe()));
    connect(clearSelectionsAct, SIGNAL(triggered()), this, SLOT(clearSelections()));
}
/*!!!
void MainWindow::tabChanged(int currTab)
{
    tabPage = currTab;

}
*/
void MainWindow::enableDisableActions()
{
    DEBUG << "MainWindow::enableDisableActions" << endl;

    //!!! should also do things like set the status texts for the
    //!!! actions appropriately by context

    QDir localRepoDir;
    QDir workFolderDir;
    bool workFolderExist;
    bool localRepoExist;

    remoteRepoActionsEnabled = true;
    if (remoteRepoPath.isEmpty()) {
        remoteRepoActionsEnabled = false;
    }

    localRepoActionsEnabled = true;
    if (workFolderPath.isEmpty()) {
        localRepoActionsEnabled = false;
        workFolderExist = false;
    }

    if (!workFolderDir.exists(workFolderPath)) {
        localRepoActionsEnabled = false;
        workFolderExist = false;
    } else {
        workFolderExist = true;
    }

    if (!localRepoDir.exists(workFolderPath + "/.hg")) {
        localRepoActionsEnabled = false;
        localRepoExist = false;
    }

    hgCloneFromRemoteAct -> setEnabled(remoteRepoActionsEnabled);
    hgIncomingAct -> setEnabled(remoteRepoActionsEnabled && remoteRepoActionsEnabled);
    hgPullAct -> setEnabled(remoteRepoActionsEnabled && remoteRepoActionsEnabled);
    hgPushAct -> setEnabled(remoteRepoActionsEnabled && remoteRepoActionsEnabled);
/*
    if (tabPage != WORKTAB)
    {
        localRepoActionsEnabled = false;
    }
*/
    bool haveDiff = (diffBinaryName != "");

    hgInitAct -> setEnabled((localRepoExist == false) && (workFolderExist==true));
    hgRefreshAct -> setEnabled(localRepoActionsEnabled);
    hgFileDiffAct -> setEnabled(localRepoActionsEnabled && haveDiff);
    hgFolderDiffAct -> setEnabled(localRepoActionsEnabled && haveDiff);
    hgRevertAct -> setEnabled(localRepoActionsEnabled);
    hgAddAct -> setEnabled(localRepoActionsEnabled);
    hgRemoveAct -> setEnabled(localRepoActionsEnabled);
    hgUpdateAct -> setEnabled(localRepoActionsEnabled);
    hgCommitAct -> setEnabled(localRepoActionsEnabled);
    hgMergeAct -> setEnabled(localRepoActionsEnabled);
    hgRetryMergeAct -> setEnabled(localRepoActionsEnabled);
    hgResolveListAct -> setEnabled(localRepoActionsEnabled);
    hgResolveMarkAct -> setEnabled(localRepoActionsEnabled);
    hgAnnotateAct -> setEnabled(localRepoActionsEnabled);
    hgServeAct -> setEnabled(localRepoActionsEnabled);
    hgTagAct -> setEnabled(localRepoActionsEnabled);
    hgIgnoreAct -> setEnabled(localRepoActionsEnabled);

    //!!!hgTabs -> enableDisableOtherTabs(tabPage);

    DEBUG << "localRepoActionsEnabled = " << localRepoActionsEnabled << endl;
    DEBUG << "canCommit = " << hgTabs->canCommit() << endl;

    //!!! new stuff:
    hgAddAct->setEnabled(localRepoActionsEnabled && hgTabs->canAdd());
    hgRemoveAct->setEnabled(localRepoActionsEnabled && hgTabs->canRemove());
    hgCommitAct->setEnabled(localRepoActionsEnabled && hgTabs->canCommit());
    hgRevertAct->setEnabled(localRepoActionsEnabled && hgTabs->canCommit());
    hgFolderDiffAct->setEnabled(localRepoActionsEnabled && hgTabs->canDoDiff());

    // A default merge makes sense if:
    //  * there is only one parent (if there are two, we have an uncommitted merge) and
    //  * there are exactly two heads that have the same branch as the current branch and
    //  * our parent is one of those heads
    //
    // A default update makes sense if:
    //  * there is only one parent and
    //  * the parent is not one of the current heads
    //!!! test this
    bool canMerge = false;
    bool canUpdate = false;
    if (currentParents.size() == 1) {
        Changeset *parent = currentParents[0];
        int currentBranchHeads = 0;
        bool parentIsHead = false;
        foreach (Changeset *head, currentHeads) {
            DEBUG << "head branch " << head->branch() << ", current branch " << currentBranch << endl;
            if (head->isOnBranch(currentBranch)) {
                ++currentBranchHeads;
                if (parent->id() == head->id()) {
                    parentIsHead = true;
                }
            }
        }
        if (currentBranchHeads == 2 && parentIsHead) {
            canMerge = true;
        }
        if (!parentIsHead) {
            canUpdate = true;
            DEBUG << "parent id = " << parent->id() << endl;
            DEBUG << " head ids "<<endl;
            foreach (Changeset *h, currentHeads) {
                DEBUG << "head id = " << h->id() << endl;
            }
        }
    }
    hgMergeAct->setEnabled(localRepoActionsEnabled && canMerge);
    hgUpdateAct->setEnabled(localRepoActionsEnabled && canUpdate);

    // Set the state field on the file status widget

    QString branchText;
    if (currentBranch == "" || currentBranch == "default") {
        branchText = tr("the default branch");
    } else {
        branchText = tr("branch \"%1\"").arg(currentBranch);
    }
    if (canUpdate) {
        hgTabs->setState(tr("On %1. Not at the head of the branch: consider updating").arg(branchText));
    } else if (canMerge) {
        hgTabs->setState(tr("<b>Awaiting merge</b> on %1").arg(branchText));
    } else {
        hgTabs->setState(tr("At the head of %1").arg(branchText));
    }
}

void MainWindow::createActions()
{
    //File actions
    hgInitAct = new QAction(tr("Init local repository"), this);
    hgInitAct->setStatusTip(tr("Create an empty local repository in selected folder"));

    hgCloneFromRemoteAct = new QAction(tr("Clone from remote"), this);
    hgCloneFromRemoteAct->setStatusTip(tr("Clone from remote repository into local repository in selected folder"));

    openAct = new QAction(QIcon(":/images/fileopen.png"), tr("Open..."), this);
    openAct -> setStatusTip(tr("Open repository"));
    openAct -> setIconVisibleInMenu(true);

    settingsAct = new QAction(QIcon(":/images/settings.png"), tr("Settings..."), this);
    settingsAct -> setStatusTip(tr("View and change application settings"));
    settingsAct -> setIconVisibleInMenu(true);

    exitAct = new QAction(QIcon(":/images/exit.png"), tr("Exit"), this);
    exitAct->setShortcuts(QKeySequence::Quit);
    exitAct->setStatusTip(tr("Exit application"));
    exitAct -> setIconVisibleInMenu(true);

    //Repository actions
    hgRefreshAct = new QAction(QIcon(":/images/status.png"), tr("Refresh"), this);
    hgRefreshAct->setStatusTip(tr("Refresh (info of) status of workfolder files"));

    hgIncomingAct = new QAction(QIcon(":/images/incoming.png"), tr("Preview"), this);
    hgIncomingAct -> setStatusTip(tr("View info of changesets incoming to us from remote repository (on pull operation)"));

    hgPullAct = new QAction(QIcon(":/images/pull.png"), tr("Pull"), this);
    hgPullAct -> setStatusTip(tr("Pull changesets from remote repository to local repository"));

    hgPushAct = new QAction(QIcon(":/images/push.png"), tr("Push"), this);
    hgPushAct->setStatusTip(tr("Push local changesets to remote repository"));

    //Workfolder actions
    hgFileDiffAct   = new QAction(QIcon(":/images/diff.png"), tr("Diff"), this);
    hgFileDiffAct->setStatusTip(tr("Filediff: View differences between selected working folder file and local repository file"));

    hgFolderDiffAct   = new QAction(QIcon(":/images/folderdiff.png"), tr("Diff"), this);
    hgFolderDiffAct->setStatusTip(tr("Folderdiff: View all differences between working folder files and local repository files"));

    hgChgSetDiffAct   = new QAction(QIcon(":/images/chgsetdiff.png"), tr("View changesetdiff"), this);
    hgChgSetDiffAct->setStatusTip(tr("Change set diff: View differences between all files of 2 repository changesets"));

    hgRevertAct = new QAction(QIcon(":/images/undo.png"), tr("Revert"), this);
    hgRevertAct->setStatusTip(tr("Undo selected working folder file changes (return to local repository version)"));

    hgAddAct = new QAction(QIcon(":/images/add.png"), tr("Add"), this);
    hgAddAct -> setStatusTip(tr("Add working folder file(s) (selected or all yet untracked) to local repository (on next commit)"));

    hgRemoveAct = new QAction(QIcon(":/images/remove.png"), tr("Remove"), this);
    hgRemoveAct -> setStatusTip(tr("Remove selected working folder file from local repository (on next commit)"));

    hgUpdateAct = new QAction(QIcon(":/images/update.png"), tr("Update"), this);
    hgUpdateAct->setStatusTip(tr("Update working folder from local repository"));

    hgCommitAct = new QAction(QIcon(":/images/commit.png"), tr("Commit"), this);
    hgCommitAct->setStatusTip(tr("Save selected file(s) or all changed files in working folder (and all subfolders) to local repository"));

    hgMergeAct = new QAction(QIcon(":/images/merge.png"), tr("Merge"), this);
    hgMergeAct->setStatusTip(tr("Merge two local repository changesets to working folder"));

    //Advanced actions
    hgUpdateToRevAct = new QAction(tr("Update to revision"), this);
    hgUpdateToRevAct -> setStatusTip(tr("Update working folder to version selected from history list"));

    hgAnnotateAct = new QAction(tr("Annotate"), this);
    hgAnnotateAct -> setStatusTip(tr("Show line-by-line version information for selected file"));

    hgResolveListAct = new QAction(tr("Resolve (list)"), this);
    hgResolveListAct -> setStatusTip(tr("Resolve (list): Show list of files needing merge"));

    hgResolveMarkAct = new QAction(tr("Resolve (mark)"), this);
    hgResolveMarkAct -> setStatusTip(tr("Resolve (mark): Mark selected file status as resolved"));

    hgRetryMergeAct = new QAction(tr("Retry merge"), this);
    hgRetryMergeAct -> setStatusTip(tr("Retry merge after failed merge attempt."));

    hgTagAct = new QAction(tr("Tag revision"), this);
    hgTagAct -> setStatusTip(tr("Give decsriptive name (tag) to current workfolder parent revision."));

    hgIgnoreAct = new QAction(tr("Edit .hgignore"), this);
    hgIgnoreAct -> setStatusTip(tr("Edit .hgignore file (file contains names of files that should be ignored by mercurial)"));

    hgServeAct = new QAction(tr("Serve (via http)"), this);
    hgServeAct -> setStatusTip(tr("Serve local repository via http for workgroup access"));

    //Help actions
    aboutAct = new QAction(tr("About"), this);
    aboutAct->setStatusTip(tr("Show the application's About box"));

    aboutQtAct = new QAction(tr("About Qt"), this);
    aboutQtAct->setStatusTip(tr("Show the Qt library's About box"));

    // Miscellaneous
    clearSelectionsAct = new QAction(tr("Clear selections"), this);
    clearSelectionsAct->setShortcut(Qt::Key_Escape);
}

void MainWindow::createMenus()
{
    fileMenu = menuBar()->addMenu(tr("File"));
    fileMenu -> addAction(hgInitAct);
    fileMenu -> addAction(hgCloneFromRemoteAct);
    fileMenu->addAction(clearSelectionsAct); //!!! can't live here!
    fileMenu -> addSeparator();
    fileMenu -> addAction(openAct);
    fileMenu -> addAction(settingsAct);
    fileMenu -> addSeparator();
    fileMenu -> addAction(exitAct);

    advancedMenu = menuBar()->addMenu(tr("Advanced"));
    advancedMenu -> addAction(hgUpdateToRevAct);
    advancedMenu -> addSeparator();
    advancedMenu -> addAction(hgAnnotateAct);
    advancedMenu -> addSeparator();
    advancedMenu -> addAction(hgRetryMergeAct);
    advancedMenu -> addAction(hgResolveListAct);
    advancedMenu -> addAction(hgResolveMarkAct);
    advancedMenu -> addSeparator();
    advancedMenu -> addAction(hgTagAct);
    advancedMenu -> addSeparator();
    advancedMenu -> addAction(hgIgnoreAct);
    advancedMenu -> addSeparator();
    advancedMenu -> addAction(hgServeAct);

    helpMenu = menuBar()->addMenu(tr("Help"));
    helpMenu->addAction(aboutAct);
    helpMenu->addAction(aboutQtAct);
}

void MainWindow::createToolBars()
{
    fileToolBar = addToolBar(tr("File"));
    fileToolBar -> setIconSize(QSize(MY_ICON_SIZE, MY_ICON_SIZE));
    fileToolBar -> addAction(openAct);
    fileToolBar -> addAction(hgRefreshAct);
    fileToolBar -> addSeparator();
//    fileToolBar -> addAction(hgChgSetDiffAct);
    fileToolBar -> setMovable(false);

    repoToolBar = addToolBar(tr(REPOMENU_TITLE));
    repoToolBar -> setIconSize(QSize(MY_ICON_SIZE, MY_ICON_SIZE));
    repoToolBar->addAction(hgIncomingAct);
    repoToolBar->addAction(hgPullAct);
    repoToolBar->addAction(hgPushAct);
    repoToolBar -> setMovable(false);

    workFolderToolBar = addToolBar(tr(WORKFOLDERMENU_TITLE));
    addToolBar(Qt::LeftToolBarArea, workFolderToolBar);
    workFolderToolBar -> setIconSize(QSize(MY_ICON_SIZE, MY_ICON_SIZE));
//    workFolderToolBar->addSeparator();
//    workFolderToolBar->addAction(hgFileDiffAct);
    workFolderToolBar->addAction(hgFolderDiffAct);
    workFolderToolBar->addSeparator();
    workFolderToolBar->addAction(hgRevertAct);
    workFolderToolBar->addAction(hgUpdateAct);
    workFolderToolBar->addAction(hgCommitAct);
    workFolderToolBar->addAction(hgMergeAct);
    workFolderToolBar->addSeparator();
    workFolderToolBar->addAction(hgAddAct);
    workFolderToolBar->addAction(hgRemoveAct);
    workFolderToolBar -> setMovable(false);

    foreach (QToolButton *tb, findChildren<QToolButton *>()) {
        tb->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    }
}


void MainWindow::createStatusBar()
{
    statusBar()->showMessage(tr("Ready"));
}


//!!! review these:

void MainWindow::readSettings()
{
    QDir workFolder;

    QSettings settings;

    remoteRepoPath = settings.value("remoterepopath", "").toString();
    workFolderPath = settings.value("workfolderpath", "").toString();
    if (!workFolder.exists(workFolderPath))
    {
        workFolderPath = "";
    }

    QPoint pos = settings.value("pos", QPoint(200, 200)).toPoint();
    QSize size = settings.value("size", QSize(400, 400)).toSize();
    firstStart = settings.value("firststart", QVariant(true)).toBool();

//!!!    initialFileTypesBits = (unsigned char) settings.value("viewFileTypes", QVariant(DEFAULT_HG_STAT_BITS)).toInt();
    resize(size);
    move(pos);
}


void MainWindow::writeSettings()
{
    QSettings settings;
    settings.setValue("pos", pos());
    settings.setValue("size", size());
    settings.setValue("remoterepopath", remoteRepoPath);
    settings.setValue("workfolderpath", workFolderPath);
    settings.setValue("firststart", firstStart);
    //!!!settings.setValue("viewFileTypes", hgTabs -> getFileTypesBits());
}




