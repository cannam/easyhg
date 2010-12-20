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
#include <QRegExp>

#include "mainwindow.h"
#include "multichoicedialog.h"
#include "startupdialog.h"
#include "colourset.h"
#include "debug.h"
#include "logparser.h"
#include "confirmcommentdialog.h"
#include "incomingdialog.h"
#include "settingsdialog.h"


MainWindow::MainWindow(QString myDirPath) :
    m_myDirPath(myDirPath)
{
    QString wndTitle;

    fsWatcher = 0;
    commitsSincePush = 0;
    shouldHgStat = true;

    createActions();
    createMenus();
    createToolBars();
    createStatusBar();

    runner = new HgRunner(myDirPath, this);
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
    connectTabsSignals();
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

    (void)findDiffBinaryName();
    (void)findMergeBinaryName();
    (void)findEditorBinaryName();

    ColourSet *cs = ColourSet::instance();
    cs->clearDefaultNames();
    cs->addDefaultName("");
    cs->addDefaultName("default");
    cs->addDefaultName(getUserInfo());

    if (workFolderPath == "") {
        open();
    }

    hgTest();
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
                         "Mercurial</a> version control system.</p>"
                         "<h4>Credits and Copyright</h4>"
                         "<p>Development carried out by Chris Cannam for "
                         "SoundSoftware.ac.uk at the Centre for Digital Music, "
                         "Queen Mary, University of London.</p>"
                         "<p>EasyMercurial is based on HgExplorer by "
                         "Jari Korhonen, with thanks.</p>"
                         "<p style=\"margin-left: 2em;\">"
                         "Copyright &copy; 2010 Queen Mary, University of London.<br>"
                         "Copyright &copy; 2010 Jari Korhonen.<br>"
                         "Copyright &copy; 2010 Chris Cannam."
                         "</p>"
                         "<p style=\"margin-left: 2em;\">"
                         "This program requires Mercurial, by Matt Mackall and others.<br>"
                         "This program uses Qt by Nokia.<br>"
                         "This program uses Nuvola icons by David Vignoni.<br>"
                         "This program may use KDiff3 by Joachim Eibl.<br>"
                         "This program may use PyQt by River Bank Computing.<br>"
                         "Packaging for Mercurial and other dependencies on Windows is derived from TortoiseHg by Steve Borho and others."
                         "</p>"
                         "<h4>License</h4>"
                         "<p>This program is free software; you can redistribute it and/or "
                         "modify it under the terms of the GNU General Public License as "
                         "published by the Free Software Foundation; either version 2 of the "
                         "License, or (at your option) any later version.  See the file "
                         "COPYING included with this distribution for more information.</p>"));
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

void MainWindow::hgTest()
{
    QStringList params;
    params << "--version";
    runner->requestAction(HgAction(ACT_TEST_HG, m_myDirPath, params));
}

void MainWindow::hgStat()
{
    QStringList params;
    params << "stat" << "-ardum";

    lastStatOutput = "";

    // annoyingly, hg stat actually modifies the working directory --
    // it creates files called hg-checklink and hg-checkexec to test
    // properties of the filesystem
    if (fsWatcher) fsWatcher->blockSignals(true);

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
    // On empty repos, "hg heads" will fail -- we don't care about
    // that.  Use --closed option so as to include closed branches;
    // otherwise we'll be stuck if the user updates into one, and our
    // incremental log will end up with spurious stuff in it because
    // we won't be pruning at the ends of closed branches
    params << "heads" << "--closed";
    runner->requestAction(HgAction(ACT_QUERY_HEADS, workFolderPath, params));
}

void MainWindow::hgLog()
{
    QStringList params;
    params << "log";
    params << "--template";
    params << Changeset::getLogTemplate();
    
    runner->requestAction(HgAction(ACT_LOG, workFolderPath, params));
}

void MainWindow::hgLogIncremental(QStringList prune)
{
    QStringList params;
    params << "log";

    foreach (QString p, prune) {
        params << "--prune" << Changeset::hashOf(p);
    }
        
    params << "--template";
    params << Changeset::getLogTemplate();
    
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
}

void MainWindow::hgCommit()
{
    QStringList params;
    QString comment;

    if (justMerged) {
        comment = mergeCommitComment;
    }

    QStringList files = hgTabs->getSelectedCommittableFiles();
    QStringList reportFiles = files;
    if (reportFiles.empty()) reportFiles = hgTabs->getAllCommittableFiles();

    QString cf(tr("Commit files"));

    if (ConfirmCommentDialog::confirmAndGetLongComment
        (this,
         cf,
         tr("<h3>%1</h3><p>%2").arg(cf)
         .arg(tr("You are about to commit the following files:")),
         tr("<h3>%1</h3><p>%2").arg(cf)
         .arg(tr("You are about to commit %n file(s).", "", reportFiles.size())),
         reportFiles,
         comment)) {

        if (!justMerged && !files.empty()) {
            // User wants to commit selected file(s) (and this is not
            // merge commit, which would fail if we selected files)
            params << "commit" << "--message" << comment
                   << "--user" << getUserInfo() << "--" << files;
        } else {
            // Commit all changes
            params << "commit" << "--message" << comment
                   << "--user" << getUserInfo();
        }
        
        runner->requestAction(HgAction(ACT_COMMIT, workFolderPath, params));
        mergeCommitComment = "";
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


void MainWindow::hgTag(QString id)
{
    QStringList params;
    QString tag;

    if (ConfirmCommentDialog::confirmAndGetShortComment
        (this,
         tr("Tag"),
         tr("Enter tag:"),
         tag)) {
        if (!tag.isEmpty()) {//!!! do something better if it is empty

            params << "tag" << "--user" << getUserInfo();
            params << "--rev" << Changeset::hashOf(id) << filterTag(tag);
            
            runner->requestAction(HgAction(ACT_TAG, workFolderPath, params));
        }
    }
}


void MainWindow::hgIgnore()
{
    QString hgIgnorePath;
    QStringList params;
    
    hgIgnorePath = workFolderPath;
    hgIgnorePath += "/.hgignore";
    
    params << hgIgnorePath;
    
    QString editor = findEditorBinaryName();

    if (editor == "") {
        DEBUG << "Failed to find a text editor" << endl;
        //!!! visible error!
        return;
    }

    HgAction action(ACT_HG_IGNORE, workFolderPath, params);
    action.executable = editor;

    runner->requestAction(action);
}

QString MainWindow::findDiffBinaryName()
{
    QSettings settings;
    settings.beginGroup("Locations");
    QString diff = settings.value("extdiffbinary", "").toString();
    if (diff == "") {
        QStringList bases;
        bases << "opendiff" << "kompare" << "kdiff3" << "meld";
        bool found = false;
        foreach (QString base, bases) {
            diff = findInPath(base, m_myDirPath, true);
            if (diff != base && diff != base + ".exe") {
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
    return diff;
}

QString MainWindow::findMergeBinaryName()
{
    QSettings settings;
    settings.beginGroup("Locations");
    QString merge = settings.value("mergebinary", "").toString();
    if (merge == "") {
        QStringList bases;
        bases << "fmdiff3" << "meld" << "diffuse" << "kdiff3";
        bool found = false;
        foreach (QString base, bases) {
            merge = findInPath(base, m_myDirPath, true);
            if (merge != base) {
                found = true;
                break;
            }
        }
        if (found) {
            settings.setValue("mergebinary", merge);
        } else {
            merge = "";
        }
    }
    return merge;
}

QString MainWindow::findEditorBinaryName()
{
    QSettings settings;
    settings.beginGroup("Locations");
    QString editor = settings.value("editorbinary", "").toString();
    if (editor == "") {
        QStringList bases;
        bases
#if defined Q_OS_WIN32
            << "wordpad.exe"
            << "C:\\Program Files\\Windows NT\\Accessories\\wordpad.exe"
            << "notepad.exe"
#elif defined Q_OS_MAC
            << "textedit"
#else
            << "gedit" << "kate"
#endif
            ;
        bool found = false;
        foreach (QString base, bases) {
            editor = findInPath(base, m_myDirPath, true);
            if (editor != base && editor != base + ".exe") {
                found = true;
                break;
            }
        }
        if (found) {
            settings.setValue("editorbinary", editor);
        } else {
            editor = "";
        }
    }
    return editor;
}

void MainWindow::hgShowSummary()
{
    QStringList params;
    
    params << "diff" << "--stat";

    runner->requestAction(HgAction(ACT_DIFF_SUMMARY, workFolderPath, params));
}

void MainWindow::hgFolderDiff()
{
    QString diff = findDiffBinaryName();
    if (diff == "") return;

    QStringList params;

    // Diff parent against working folder (folder diff)

    params << "--config" << "extensions.extdiff=" << "extdiff";
    params << "--program" << diff;

    params << hgTabs->getSelectedCommittableFiles(); // may be none: whole dir

    runner->requestAction(HgAction(ACT_FOLDERDIFF, workFolderPath, params));
}


void MainWindow::hgDiffToCurrent(QString id)
{
    QString diff = findDiffBinaryName();
    if (diff == "") return;

    QStringList params;

    // Diff given revision against working folder

    params << "--config" << "extensions.extdiff=" << "extdiff";
    params << "--program" << diff;
    params << "--rev" << Changeset::hashOf(id);

    runner->requestAction(HgAction(ACT_FOLDERDIFF, workFolderPath, params));
}


void MainWindow::hgDiffToParent(QString child, QString parent)
{
    QString diff = findDiffBinaryName();
    if (diff == "") return;

    QStringList params;

    // Diff given revision against working folder

    params << "--config" << "extensions.extdiff=" << "extdiff";
    params << "--program" << diff;
    params << "--rev" << Changeset::hashOf(parent)
           << "--rev" << Changeset::hashOf(child);

    runner->requestAction(HgAction(ACT_CHGSETDIFF, workFolderPath, params));
}


void MainWindow::hgUpdate()
{
    QStringList params;

    params << "update";
    
    runner->requestAction(HgAction(ACT_UPDATE, workFolderPath, params));
}


void MainWindow::hgUpdateToRev(QString id)
{
    QStringList params;

    params << "update" << "--rev" << Changeset::hashOf(id) << "--check";

    runner->requestAction(HgAction(ACT_UPDATE, workFolderPath, params));
}


void MainWindow::hgRevert()
{
    QStringList params;
    QString comment;

    QStringList files = hgTabs->getSelectedRevertableFiles();
    if (files.empty()) files = hgTabs->getAllRevertableFiles();

    QString rf(tr("Revert files"));
    
    if (ConfirmCommentDialog::confirmDangerousFilesAction
        (this,
         rf,
         tr("<h3>%1</h3><p>%2").arg(rf)
         .arg(tr("You are about to <b>revert</b> the following files to their previous committed state.<br><br>This will <b>throw away any changes</b> that you have made to these files but have not committed:")),
         tr("<h3>%1</h3><p>%2").arg(rf)
         .arg(tr("You are about to <b>revert</b> %n file(s).<br><br>This will <b>throw away any changes</b> that you have made to these files but have not committed.", "", files.size())),
         files)) {

        lastRevertedFiles = files;
        
        if (files.empty()) {
            params << "revert" << "--no-backup";
        } else {
            params << "revert" << "--" << files;
        }

        //!!! This is problematic.  If you've got an uncommitted
        //!!! merge, you can't revert it without declaring which
        //!!! parent of the merge you want to revert to (reasonably
        //!!! enough).  We're OK if we just did the merge in easyhg a
        //!!! moment ago, because we have a record of which parent was
        //!!! the target -- but if you exit and restart, we've lost
        //!!! that record and it doesn't appear to be possible to get
        //!!! it back from Hg.  Even if you just switched from one
        //!!! repo to another, the record is lost.  What to do?

        if (justMerged && mergeTargetRevision != "") {
            params << "--rev" << mergeTargetRevision;
        }
        
        runner->requestAction(HgAction(ACT_REVERT, workFolderPath, params));
    }
}


void MainWindow::hgMarkResolved(QStringList files)
{
    QStringList params;

    params << "resolve" << "--mark";

    if (files.empty()) {
        params << "--all";
    } else {
        params << "--" << files;
    }

    runner->requestAction(HgAction(ACT_RESOLVE_MARK, workFolderPath, params));
}


void MainWindow::hgRetryMerge()
{
    QStringList params;

    params << "resolve";

    QString merge = findMergeBinaryName();
    if (merge != "") {
        params << "--tool" << merge;
    }

    QStringList files = hgTabs->getSelectedUnresolvedFiles();
    if (files.empty()) {
        params << "--all";
    } else {
        params << "--" << files;
    }

    runner->requestAction(HgAction(ACT_RETRY_MERGE, workFolderPath, params));

    mergeCommitComment = tr("Merge");
}


void MainWindow::hgMerge()
{
    if (hgTabs->canResolve()) {
        hgRetryMerge();
        return;
    }

    QStringList params;

    params << "merge";

    QString merge = findMergeBinaryName();
    if (merge != "") {
        params << "--tool" << merge;
    }

    if (currentParents.size() == 1) {
        mergeTargetRevision = currentParents[0]->id();
    }

    runner->requestAction(HgAction(ACT_MERGE, workFolderPath, params));

    mergeCommitComment = tr("Merge");
}


void MainWindow::hgMergeFrom(QString id)
{
    QStringList params;

    params << "merge";
    params << "--rev" << Changeset::hashOf(id);

    QString merge = findMergeBinaryName();
    if (merge != "") {
        params << "--tool" << merge;
    }
    
    runner->requestAction(HgAction(ACT_MERGE, workFolderPath, params));

    mergeCommitComment = "";

    foreach (Changeset *cs, currentHeads) {
        if (cs->id() == id && !cs->isOnBranch(currentBranch)) {
            if (cs->branch() == "" || cs->branch() == "default") {
                mergeCommitComment = tr("Merge from the default branch");
            } else {
                mergeCommitComment = tr("Merge from branch \"%1\"").arg(cs->branch());
            }
        }
    }

    if (mergeCommitComment == "") {
        mergeCommitComment = tr("Merge from %1").arg(id);
    }
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
    params << "--template" << Changeset::getLogTemplate();

    runner->requestAction(HgAction(ACT_INCOMING, workFolderPath, params));
}

static QMessageBox::StandardButton confirm(QWidget *parent,
                                           QString title,
                                           QString text,
                                           QString okButtonText)
{
    QMessageBox box(QMessageBox::Question,
                    title,
                    text,
                    QMessageBox::Cancel,
                    parent);

    QPushButton *ok = box.addButton(QMessageBox::Ok);
    ok->setText(okButtonText);
    if (box.exec() == -1) return QMessageBox::Cancel;
    return box.standardButton(box.clickedButton());
}

void MainWindow::hgPull()
{
    if (confirm
        (this, tr("Confirm pull"),
         format3(tr("Confirm pull from remote repository"),
                 tr("You are about to pull changes from the following remote repository:"),
                 remoteRepoPath),
         tr("Pull")) == QMessageBox::Ok) {

        QStringList params;
        params << "pull" << remoteRepoPath;
        runner->requestAction(HgAction(ACT_PULL, workFolderPath, params));
    }
}

void MainWindow::hgPush()
{
    if (confirm
        (this, tr("Confirm push"),
         format3(tr("Confirm push to remote repository"),
                 tr("You are about to push your changes to the following remote repository:"),
                 remoteRepoPath),
         tr("Push")) == QMessageBox::Ok) {

        QStringList params;
        params << "push" << "--new-branch" << remoteRepoPath;
        runner->requestAction(HgAction(ACT_PUSH, workFolderPath, params));
    }
}

QStringList MainWindow::listAllUpIpV4Addresses()
{
    QStringList ret;
    QList<QNetworkInterface> ifaces = QNetworkInterface::allInterfaces();

    for (int i = 0; i < ifaces.count(); i++) {
        QNetworkInterface iface = ifaces.at(i);
        if (iface.flags().testFlag(QNetworkInterface::IsUp)
            && !iface.flags().testFlag(QNetworkInterface::IsLoopBack)) {
            for (int j=0; j<iface.addressEntries().count(); j++) {
                QHostAddress tmp = iface.addressEntries().at(j).ip();
                if (QAbstractSocket::IPv4Protocol == tmp.protocol()) {
                    ret.push_back(tmp.toString());
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
    lastStatOutput = "";
    lastRevertedFiles.clear();
    mergeTargetRevision = "";
    mergeCommitComment = "";
    stateUnknown = true;
    needNewLog = true;
}

void MainWindow::hgServe()
{
    QStringList params;
    QString msg;

    QStringList addrs = listAllUpIpV4Addresses();

    if (addrs.empty()) {
        QMessageBox::critical
            (this, tr("Serve"), tr("Failed to identify an active IPv4 address"));
        return;
    }

    //!!! should find available port as well

    QTextStream ts(&msg);
    ts << QString("<qt><p>%1</p>")
        .arg(tr("Running temporary server at %n address(es):", "", addrs.size()));
    foreach (QString addr, addrs) {
        ts << QString("<pre>&nbsp;&nbsp;http://%1:8000</pre>").arg(xmlEncode(addr));
    }
    ts << tr("<p>Press Close to stop the server and return.</p>");
    ts.flush();
             
    params << "serve";

    runner->requestAction(HgAction(ACT_SERVE, workFolderPath, params));
    
    QMessageBox::information(this, tr("Serve"), msg, QMessageBox::Close);

    runner->killCurrentActions();
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

void MainWindow::changeRemoteRepo()
{
    // This will involve rewriting the local .hgrc

    QDir hgDir(workFolderPath + "/.hg");
    if (!hgDir.exists()) {
        //!!! visible error!
        return;
    }

    QFileInfo hgrc(workFolderPath + "/.hg/hgrc");
    if (hgrc.exists() && !hgrc.isWritable()) {
        //!!! visible error!
        return;
    }

    MultiChoiceDialog *d = new MultiChoiceDialog
        (tr("Change Remote Location"),
         tr("<qt><big>Change the remote location</big></qt>"),
         this);

    d->addChoice("remote",
                 tr("<qt><center><img src=\":images/browser-64.png\"><br>Remote repository</center></qt>"),
                 tr("Provide a new URL to use for push and pull actions from the current local repository."),
                 MultiChoiceDialog::UrlArg);

    if (d->exec() == QDialog::Accepted) {
        QSettings s(hgrc.canonicalFilePath(), QSettings::IniFormat);
        s.beginGroup("paths");
        s.setValue("default", d->getArgument());
        stateUnknown = true;
        hgQueryPaths();
    }

    delete d;
}

void MainWindow::open(QString local)
{
    if (openLocal(local)) {
        enableDisableActions();
        clearState();
        hgQueryPaths();
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
    SettingsDialog *settingsDlg = new SettingsDialog(this);
    settingsDlg->exec();
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

void MainWindow::fsDirectoryChanged(QString d)
{
    DEBUG << "MainWindow::fsDirectoryChanged " << d << endl;
    hgStat();
}

void MainWindow::fsFileChanged(QString f)
{
    DEBUG << "MainWindow::fsFileChanged " << f << endl;
    hgStat();
}

QString MainWindow::format3(QString head, QString intro, QString code)
{
    code = xmlEncode(code).replace("\n", "<br>").replace("-", "&#8209;").replace(" ", "&nbsp;");
    if (intro == "") {
        return QString("<qt><h3>%1</h3><p><code>%2</code></p>")
            .arg(head).arg(code);
    } else if (code == "") {
        return QString("<qt><h3>%1</h3><p>%2</p>")
            .arg(head).arg(intro);
    } else {
        return QString("<qt><h3>%1</h3><p>%2</p><p><code>%3</code></p>")
            .arg(head).arg(intro).arg(code);
    }
}

void MainWindow::showIncoming(QString output)
{
    runner->hide();
    IncomingDialog *d = new IncomingDialog(this, output);
    d->exec();
    delete d;
}

int MainWindow::extractChangeCount(QString text)
{
    QRegExp re("added (\\d+) ch\\w+ with (\\d+) ch\\w+ to (\\d+) f\\w+");
    if (re.indexIn(text) >= 0) {
        return re.cap(1).toInt();
    } else if (text.contains("no changes")) {
        return 0;
    } else {
        return -1; // unknown
    }
}

void MainWindow::showPushResult(QString output)
{
    QString report;
    int n = extractChangeCount(output);
    if (n > 0) {
        report = tr("Pushed %n changeset(s)", "", n);
    } else if (n == 0) {
        report = tr("No changes to push");
    } else {
        report = tr("Push complete");
    }
    report = format3(report, tr("The push command output was:"), output);
    runner->hide();
    QMessageBox::information(this, "Push complete", report);
}

void MainWindow::showPullResult(QString output)
{
    QString report;
    int n = extractChangeCount(output);
    if (n > 0) {
        report = tr("Pulled %n changeset(s)", "", n);
    } else if (n == 0) {
        report = tr("No changes to pull");
    } else {
        report = tr("Pull complete");
    }
    report = format3(report, tr("The pull command output was:"), output);
    runner->hide();

    //!!! and something about updating

    QMessageBox::information(this, "Pull complete", report);
}

void MainWindow::reportNewRemoteHeads(QString output)
{
    bool headsAreLocal = false;

    if (currentParents.size() == 1) {
        int currentBranchHeads = 0;
        bool parentIsHead = false;
        Changeset *parent = currentParents[0];
        foreach (Changeset *head, currentHeads) {
            if (head->isOnBranch(currentBranch)) {
                ++currentBranchHeads;
            }
            if (parent->id() == head->id()) {
                parentIsHead = true;
            }
        }
        if (currentBranchHeads == 2 && parentIsHead) {
            headsAreLocal = true;
        }
    }

    if (headsAreLocal) {
        QMessageBox::warning
            (this, tr("Push failed"),
             format3(tr("Push failed"),
                     tr("Your local repository could not be pushed to the remote repository.<br><br>You may need to merge the changes locally first.<br><br>The output of the push command was:"),
                     output));
    } else {
        QMessageBox::warning
            (this, tr("Push failed"),
             format3(tr("Push failed"),
                     tr("Your local repository could not be pushed to the remote repository.<br><br>The remote repository may have been changed by someone else since you last pushed. Try pulling and merging their changes into your local repository first.<br><br>The output of the push command was:"),
                     output));
    }
}

void MainWindow::commandFailed(HgAction action, QString output)
{
    DEBUG << "MainWindow::commandFailed" << endl;

    // Some commands we just have to ignore bad return values from:

    switch(action.action) {
    case ACT_NONE:
        // uh huh
        return;
    case ACT_TEST_HG:
        QMessageBox::warning
            (this, tr("Failed to run Mercurial"),
             format3(tr("Failed to run Mercurial"),
                     tr("The Mercurial program either could not be found or failed to run.<br>This may indicate a problem with the Mercurial installation, or with the EasyHg interaction extension.<br><br>%1").arg(output == "" ? QString("") : tr("The test command said:")),
                     output));
        settings();
        return;
    case ACT_INCOMING:
        // returns non-zero code if the check was successful but there
        // are no changes pending
        if (output.trimmed() == "") showIncoming("");
        return;
    case ACT_QUERY_HEADS:
        // fails if repo is empty; we don't care (if there's a genuine
        // problem, something else will fail too).  Need to do this,
        // otherwise empty repo state will not be reflected properly
        // (since heads/log procedure never completes for empty repo)
        enableDisableActions();
        return;
    case ACT_FOLDERDIFF:
    case ACT_CHGSETDIFF:
        // external program, unlikely to be anything useful in stderr
        // and some return with failure codes when something as basic
        // as the user closing the window via the wm happens
        return;
    case ACT_PUSH:
        if (output.contains("creates new remote heads")) {
            reportNewRemoteHeads(output);
            return;
        }
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
        .arg((output.trimmed() != "") ?
             tr("<p>Its output said:</p><code>%1</code>")
             .arg(xmlEncode(output.left(800))
                  .replace("\n", "<br>"))
             : "");

    QMessageBox::warning(this, tr("Command failed"), message);
}

void MainWindow::commandCompleted(HgAction completedAction, QString output)
{
    HGACTIONS action = completedAction.action;

    if (action == ACT_NONE) return;

    bool headsChanged = false;
    QStringList oldHeadIds;

    switch (action) {

    case ACT_TEST_HG:
        break;

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
        if (fsWatcher) fsWatcher->blockSignals(false);
        lastStatOutput = output;
        updateFileSystemWatcher();
        break;

    case ACT_RESOLVE_LIST:
        if (output != "") {
            // Remove lines beginning with R (they are resolved,
            // and the file stat parser treats R as removed)
            QStringList outList = output.split('\n');
            QStringList winnowed;
            foreach (QString line, outList) {
                if (!line.startsWith("R ")) winnowed.push_back(line);
            }
            output = winnowed.join("\n");
        }
        DEBUG << "lastStatOutput = " << lastStatOutput << endl;
        DEBUG << "output = " << output << endl;
        hgTabs->updateWorkFolderFileList(lastStatOutput + output);
        break;

    case ACT_RESOLVE_MARK:
        shouldHgStat = true;
        break;
        
    case ACT_INCOMING:
        showIncoming(output);
        break;

    case ACT_ANNOTATE:
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
    {
        foreach (Changeset *cs, currentParents) delete cs;
        currentParents = Changeset::parseChangesets(output);
        QStringList parentIds = Changeset::getIds(currentParents);
        hgTabs->setCurrent(parentIds, currentBranch);
    }
        break;
        
    case ACT_QUERY_HEADS:
    {
        oldHeadIds = Changeset::getIds(currentHeads);
        Changesets newHeads = Changeset::parseChangesets(output);
        QStringList newHeadIds = Changeset::getIds(newHeads);
        if (oldHeadIds != newHeadIds) {
            DEBUG << "Heads changed, will prompt an incremental log if appropriate" << endl;
            headsChanged = true;
            foreach (Changeset *cs, currentHeads) delete cs;
            currentHeads = newHeads;
        }
    }
        break;

    case ACT_COMMIT:
        hgTabs->clearSelections();
        justMerged = false;
        shouldHgStat = true;
        break;

    case ACT_REVERT:
        hgMarkResolved(lastRevertedFiles);
        justMerged = false;
        break;
        
    case ACT_REMOVE:
    case ACT_ADD:
        hgTabs->clearSelections();
        shouldHgStat = true;
        break;

    case ACT_TAG:
        needNewLog = true;
        shouldHgStat = true;
        break;

    case ACT_DIFF_SUMMARY:
        QMessageBox::information(this, tr("Change summary"),
                                 format3(tr("Summary of uncommitted changes"),
                                         "",
                                         output));
        break;

    case ACT_FOLDERDIFF:
    case ACT_CHGSETDIFF:
    case ACT_SERVE:
    case ACT_HG_IGNORE:
        shouldHgStat = true;
        break;
        
    case ACT_UPDATE:
        QMessageBox::information(this, tr("Update"), tr("<qt><h3>Update successful</h3><p>%1</p>").arg(xmlEncode(output)));
        shouldHgStat = true;
        break;
        
    case ACT_MERGE:
        QMessageBox::information(this, tr("Update"), tr("<qt><h3>Merge successful</h3><p>%1</p>").arg(xmlEncode(output)));
        shouldHgStat = true;
        justMerged = true;
        break;
        
    case ACT_RETRY_MERGE:
        QMessageBox::information(this, tr("Resolved"),
                                 tr("<qt><h3>Merge resolved</h3><p>Merge resolved successfully.</p>"));
        shouldHgStat = true;
        justMerged = true;
        break;
        
    default:
        break;
    }

    // Sequence when no full log required:
    //   paths -> branch -> stat -> resolve-list -> heads ->
    //     incremental-log (only if heads changed) -> parents
    // 
    // Sequence when full log required:
    //   paths -> branch -> stat -> resolve-list -> heads -> parents -> log
    //
    // Note we want to call enableDisableActions only once, at the end
    // of whichever sequence is in use.

    bool noMore = false;

    switch (action) {

    case ACT_TEST_HG:
        hgQueryPaths();
        break;
        
    case ACT_QUERY_PATHS:
        hgQueryBranch();
        break;

    case ACT_QUERY_BRANCH:
        hgStat();
        break;
        
    case ACT_STAT:
        hgResolveList();
        break;

    case ACT_RESOLVE_LIST:
        hgQueryHeads();
        break;

    case ACT_QUERY_HEADS:
        if (headsChanged && !needNewLog) {
            hgLogIncremental(oldHeadIds);
        } else {
            hgQueryParents();
        }
        break;

    case ACT_LOG_INCREMENTAL:
        hgQueryParents();
        break;

    case ACT_QUERY_PARENTS:
        if (needNewLog) {
            hgLog();
        } else {
            // we're done
            noMore = true;
        }
        break;

    case ACT_LOG:
        // we're done
        noMore = true;

    default:
        if (shouldHgStat) {
            shouldHgStat = false;
            hgQueryPaths();
        } else {
            noMore = true;
        }
        break;
    }

    if (noMore) {
        stateUnknown = false;
        enableDisableActions();
        hgTabs->updateHistory();
    }
}

void MainWindow::connectActions()
{
    connect(exitAct, SIGNAL(triggered()), this, SLOT(close()));
    connect(aboutAct, SIGNAL(triggered()), this, SLOT(about()));

    connect(hgRefreshAct, SIGNAL(triggered()), this, SLOT(hgRefresh()));
    connect(hgRemoveAct, SIGNAL(triggered()), this, SLOT(hgRemove()));
    connect(hgAddAct, SIGNAL(triggered()), this, SLOT(hgAdd()));
    connect(hgCommitAct, SIGNAL(triggered()), this, SLOT(hgCommit()));
    connect(hgFolderDiffAct, SIGNAL(triggered()), this, SLOT(hgFolderDiff()));
    connect(hgUpdateAct, SIGNAL(triggered()), this, SLOT(hgUpdate()));
    connect(hgRevertAct, SIGNAL(triggered()), this, SLOT(hgRevert()));
    connect(hgMergeAct, SIGNAL(triggered()), this, SLOT(hgMerge()));
    connect(hgIgnoreAct, SIGNAL(triggered()), this, SLOT(hgIgnore()));

    connect(settingsAct, SIGNAL(triggered()), this, SLOT(settings()));
    connect(openAct, SIGNAL(triggered()), this, SLOT(open()));
    connect(changeRemoteRepoAct, SIGNAL(triggered()), this, SLOT(changeRemoteRepo()));

    connect(hgIncomingAct, SIGNAL(triggered()), this, SLOT(hgIncoming()));
    connect(hgPullAct, SIGNAL(triggered()), this, SLOT(hgPull()));
    connect(hgPushAct, SIGNAL(triggered()), this, SLOT(hgPush()));

    connect(hgAnnotateAct, SIGNAL(triggered()), this, SLOT(hgAnnotate()));
    connect(hgServeAct, SIGNAL(triggered()), this, SLOT(hgServe()));
    connect(clearSelectionsAct, SIGNAL(triggered()), this, SLOT(clearSelections()));
}

void MainWindow::connectTabsSignals()
{
    connect(hgTabs, SIGNAL(commit()),
            this, SLOT(hgCommit()));
    
    connect(hgTabs, SIGNAL(revert()),
            this, SLOT(hgRevert()));
    
    connect(hgTabs, SIGNAL(diffWorkingFolder()),
            this, SLOT(hgFolderDiff()));
    
    connect(hgTabs, SIGNAL(showSummary()),
            this, SLOT(hgShowSummary()));

    connect(hgTabs, SIGNAL(updateTo(QString)),
            this, SLOT(hgUpdateToRev(QString)));

    connect(hgTabs, SIGNAL(diffToCurrent(QString)),
            this, SLOT(hgDiffToCurrent(QString)));

    connect(hgTabs, SIGNAL(diffToParent(QString, QString)),
            this, SLOT(hgDiffToParent(QString, QString)));

    connect(hgTabs, SIGNAL(mergeFrom(QString)),
            this, SLOT(hgMergeFrom(QString)));

    connect(hgTabs, SIGNAL(tag(QString)),
            this, SLOT(hgTag(QString)));
}    

void MainWindow::enableDisableActions()
{
    DEBUG << "MainWindow::enableDisableActions" << endl;

    //!!! should also do things like set the status texts for the
    //!!! actions appropriately by context

    QDir localRepoDir;
    QDir workFolderDir;
    bool workFolderExist = true;
    bool localRepoExist = true;

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

    hgIncomingAct -> setEnabled(remoteRepoActionsEnabled && remoteRepoActionsEnabled);
    hgPullAct -> setEnabled(remoteRepoActionsEnabled && remoteRepoActionsEnabled);
    hgPushAct -> setEnabled(remoteRepoActionsEnabled && remoteRepoActionsEnabled);

    bool haveDiff = false;
    QSettings settings;
    settings.beginGroup("Locations");
    if (settings.value("extdiffbinary", "").toString() != "") {
        haveDiff = true;
    }
    settings.endGroup();

    hgRefreshAct -> setEnabled(localRepoActionsEnabled);
    hgFolderDiffAct -> setEnabled(localRepoActionsEnabled && haveDiff);
    hgRevertAct -> setEnabled(localRepoActionsEnabled);
    hgAddAct -> setEnabled(localRepoActionsEnabled);
    hgRemoveAct -> setEnabled(localRepoActionsEnabled);
    hgUpdateAct -> setEnabled(localRepoActionsEnabled);
    hgCommitAct -> setEnabled(localRepoActionsEnabled);
    hgMergeAct -> setEnabled(localRepoActionsEnabled);
    hgAnnotateAct -> setEnabled(localRepoActionsEnabled);
    hgServeAct -> setEnabled(localRepoActionsEnabled);
    hgIgnoreAct -> setEnabled(localRepoActionsEnabled);

    DEBUG << "localRepoActionsEnabled = " << localRepoActionsEnabled << endl;
    DEBUG << "canCommit = " << hgTabs->canCommit() << endl;

    hgAddAct->setEnabled(localRepoActionsEnabled && hgTabs->canAdd());
    hgRemoveAct->setEnabled(localRepoActionsEnabled && hgTabs->canRemove());
    hgCommitAct->setEnabled(localRepoActionsEnabled && hgTabs->canCommit());
    hgRevertAct->setEnabled(localRepoActionsEnabled && hgTabs->canRevert());
    hgFolderDiffAct->setEnabled(localRepoActionsEnabled && hgTabs->canDiff());

    // A default merge makes sense if:
    //  * there is only one parent (if there are two, we have an uncommitted merge) and
    //  * there are exactly two heads that have the same branch as the current branch and
    //  * our parent is one of those heads
    //
    // A default update makes sense if:
    //  * there is only one parent and
    //  * the parent is not one of the current heads

    bool canMerge = false;
    bool canUpdate = false;
    bool haveMerge = false;
    bool emptyRepo = false;
    int currentBranchHeads = 0;

    if (currentParents.size() == 1) {
        bool parentIsHead = false;
        Changeset *parent = currentParents[0];
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
    } else if (currentParents.size() == 0) {
        emptyRepo = true;
    } else {
        haveMerge = true;
        justMerged = true;
    }
        
    hgMergeAct->setEnabled(localRepoActionsEnabled &&
                           (canMerge || hgTabs->canResolve()));
    hgUpdateAct->setEnabled(localRepoActionsEnabled &&
                            (canUpdate && !hgTabs->haveChangesToCommit()));

    // Set the state field on the file status widget

    QString branchText;
    if (currentBranch == "" || currentBranch == "default") {
        branchText = tr("the default branch");
    } else {
        branchText = tr("branch \"%1\"").arg(currentBranch);
    }

    if (stateUnknown) {
        hgTabs->setState(tr("(Examining repository)"));
    } else if (emptyRepo) {
        hgTabs->setState(tr("Nothing committed to this repository yet"));
    } else if (canMerge) {
        hgTabs->setState(tr("<b>Awaiting merge</b> on %1").arg(branchText));
    } else if (!hgTabs->getAllUnresolvedFiles().empty()) {
        hgTabs->setState(tr("Have unresolved files following merge on %1").arg(branchText));
    } else if (haveMerge) {
        hgTabs->setState(tr("Have merged but not yet committed on %1").arg(branchText));
    } else if (canUpdate) {
        if (hgTabs->haveChangesToCommit()) {
            // have uncommitted changes
            hgTabs->setState(tr("On %1. Not at the head of the branch").arg(branchText));
        } else {
            // no uncommitted changes
            hgTabs->setState(tr("On %1. Not at the head of the branch: consider updating").arg(branchText));
        }
    } else if (currentBranchHeads > 1) {
        hgTabs->setState(tr("At one of %n heads of %1", "", currentBranchHeads).arg(branchText));
    } else {
        hgTabs->setState(tr("At the head of %1").arg(branchText));
    }
}

void MainWindow::createActions()
{
    //File actions
    openAct = new QAction(QIcon(":/images/fileopen.png"), tr("Open..."), this);
    openAct -> setStatusTip(tr("Open a repository"));

    changeRemoteRepoAct = new QAction(tr("Change Remote Location..."), this);
    changeRemoteRepoAct->setStatusTip(tr("Change the default remote repository for pull and push actions"));

    settingsAct = new QAction(QIcon(":/images/settings.png"), tr("Settings..."), this);
    settingsAct -> setStatusTip(tr("View and change application settings"));

    exitAct = new QAction(QIcon(":/images/exit.png"), tr("Quit"), this);
    exitAct->setShortcuts(QKeySequence::Quit);
    exitAct->setStatusTip(tr("Quit EasyMercurial"));

    //Repository actions
    hgRefreshAct = new QAction(QIcon(":/images/status.png"), tr("Refresh"), this);
    hgRefreshAct->setStatusTip(tr("Refresh the window to show the current state of the working folder"));

    hgIncomingAct = new QAction(QIcon(":/images/incoming.png"), tr("Preview"), this);
    hgIncomingAct -> setStatusTip(tr("See what changes are available in the remote repository waiting to be pulled"));

    hgPullAct = new QAction(QIcon(":/images/pull.png"), tr("Pull"), this);
    hgPullAct -> setStatusTip(tr("Pull changes from the remote repository to the local repository"));

    hgPushAct = new QAction(QIcon(":/images/push.png"), tr("Push"), this);
    hgPushAct->setStatusTip(tr("Push changes from the local repository to the remote repository"));

    //Workfolder actions
    hgFolderDiffAct   = new QAction(QIcon(":/images/folderdiff.png"), tr("Diff"), this);
    hgFolderDiffAct->setStatusTip(tr("See what has changed in the working folder compared with the last committed state"));

    hgRevertAct = new QAction(QIcon(":/images/undo.png"), tr("Revert"), this);
    hgRevertAct->setStatusTip(tr("Throw away your changes and return to the last committed state"));

    hgAddAct = new QAction(QIcon(":/images/add.png"), tr("Add"), this);
    hgAddAct -> setStatusTip(tr("Mark the selected file(s) to be added on the next commit"));

    //!!! needs to be modified for number
    hgRemoveAct = new QAction(QIcon(":/images/remove.png"), tr("Remove"), this);
    hgRemoveAct -> setStatusTip(tr("Mark the selected file(s) to be removed from version control on the next commit"));

    hgUpdateAct = new QAction(QIcon(":/images/update.png"), tr("Update"), this);
    hgUpdateAct->setStatusTip(tr("Update the working folder to the head of the current repository branch"));

    //!!! needs to be modified when files selected
    hgCommitAct = new QAction(QIcon(":/images/commit.png"), tr("Commit"), this);
    hgCommitAct->setStatusTip(tr("Commit your changes to the local repository"));

    hgMergeAct = new QAction(QIcon(":/images/merge.png"), tr("Merge"), this);
    hgMergeAct->setStatusTip(tr("Merge the two independent sets of changes in the local repository into the working folder"));

    //Advanced actions
    //!!! needs to be modified for number
    hgAnnotateAct = new QAction(tr("Annotate"), this);
    hgAnnotateAct -> setStatusTip(tr("Show line-by-line version information for selected file"));

    hgIgnoreAct = new QAction(tr("Edit .hgignore File"), this);
    hgIgnoreAct -> setStatusTip(tr("Edit the .hgignore file, containing the names of files that should be ignored by Mercurial"));

    hgServeAct = new QAction(tr("Serve via HTTP"), this);
    hgServeAct -> setStatusTip(tr("Serve local repository via http for workgroup access"));

    //Help actions
    aboutAct = new QAction(tr("About EasyMercurial"), this);

    // Miscellaneous
    clearSelectionsAct = new QAction(tr("Clear selections"), this);
    clearSelectionsAct->setShortcut(Qt::Key_Escape);
}

void MainWindow::createMenus()
{
    fileMenu = menuBar()->addMenu(tr("File"));

    fileMenu -> addAction(openAct);
    fileMenu -> addAction(changeRemoteRepoAct);
    fileMenu -> addSeparator();

    advancedMenu = fileMenu->addMenu(tr("Advanced"));

    fileMenu -> addAction(settingsAct);

    fileMenu -> addSeparator();
    fileMenu -> addAction(exitAct);

    advancedMenu -> addAction(hgIgnoreAct);
    advancedMenu -> addAction(hgServeAct);

    helpMenu = menuBar()->addMenu(tr("Help"));
    helpMenu->addAction(aboutAct);
}

void MainWindow::createToolBars()
{
    fileToolBar = addToolBar(tr("File"));
    fileToolBar -> setIconSize(QSize(MY_ICON_SIZE, MY_ICON_SIZE));
    fileToolBar -> addAction(openAct);
    fileToolBar -> addAction(hgRefreshAct);
    fileToolBar -> addSeparator();
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




