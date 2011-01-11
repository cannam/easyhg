/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    EasyMercurial

    Based on hgExplorer by Jari Korhonen
    Copyright (c) 2010 Jari Korhonen
    Copyright (c) 2011 Chris Cannam
    Copyright (c) 2011 Queen Mary, University of London
    
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
#include <QShortcut>
#include <QUrl>
#include <QTimer>

#include "mainwindow.h"
#include "multichoicedialog.h"
#include "startupdialog.h"
#include "colourset.h"
#include "debug.h"
#include "logparser.h"
#include "confirmcommentdialog.h"
#include "incomingdialog.h"
#include "settingsdialog.h"
#include "version.h"


MainWindow::MainWindow(QString myDirPath) :
    m_myDirPath(myDirPath),
    m_fsWatcherGeneralTimer(0),
    m_fsWatcherRestoreTimer(0),
    m_fsWatcherSuspended(false)
{
    setWindowIcon(QIcon(":images/easyhg-icon.png"));

    QString wndTitle;

    showAllFiles = false;

    fsWatcher = 0;
    commitsSincePush = 0;
    shouldHgStat = true;

    createActions();
    createMenus();
    createToolBars();
    createStatusBar();

    runner = new HgRunner(myDirPath, this);
    connect(runner, SIGNAL(commandStarting(HgAction)),
            this, SLOT(commandStarting(HgAction)));
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

    QWidget *central = new QWidget(this);
    setCentralWidget(central);

    hgTabs = new HgTabWidget(central, remoteRepoPath, workFolderPath);
    connectTabsSignals();

    // Instead of setting the tab widget as our central widget
    // directly, put it in a layout, so that we can have some space
    // around it on the Mac where it looks very strange without

    QGridLayout *cl = new QGridLayout(central);
    cl->addWidget(hgTabs, 0, 0);

#ifndef Q_OS_MAC
    cl->setMargin(0);
#endif

    connect(hgTabs, SIGNAL(selectionChanged()),
            this, SLOT(enableDisableActions()));
    connect(hgTabs, SIGNAL(showAllChanged(bool)),
            this, SLOT(showAllChanged(bool)));

    setUnifiedTitleAndToolBarOnMac(true);
    connectActions();
    clearState();
    enableDisableActions();

    if (firstStart) {
        startupDialog();
    }

    SettingsDialog::findDefaultLocations(m_myDirPath);

    ColourSet *cs = ColourSet::instance();
    cs->clearDefaultNames();
    cs->addDefaultName("");
    cs->addDefaultName("default");
    cs->addDefaultName(getUserInfo());

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
                      tr("<qt><h2>EasyMercurial v%1</h2>"
#ifdef Q_OS_MAC
                         "<font size=-1>"
#endif
                         "<p>EasyMercurial is a simple user interface for the "
                         "Mercurial</a> version control system.</p>"
                         "<h4>Credits and Copyright</h4>"
                         "<p>Development carried out by Chris Cannam for "
                         "SoundSoftware.ac.uk at the Centre for Digital Music, "
                         "Queen Mary, University of London.</p>"
                         "<p>EasyMercurial is based on HgExplorer by "
                         "Jari Korhonen, with thanks.</p>"
                         "<p style=\"margin-left: 2em;\">"
                         "Copyright &copy; 2011 Queen Mary, University of London.<br>"
                         "Copyright &copy; 2010 Jari Korhonen.<br>"
                         "Copyright &copy; 2011 Chris Cannam."
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
                         "COPYING included with this distribution for more information.</p>"
#ifdef Q_OS_MAC
                         "</font>"
#endif
                          ).arg(EASYHG_VERSION));
}

void MainWindow::clearSelections()
{
    hgTabs->clearSelections();
}

void MainWindow::showAllChanged(bool s)
{
    showAllFiles = s;
    hgQueryPaths();
}

void MainWindow::hgRefresh()
{
    clearState();
    hgQueryPaths();
}

void MainWindow::hgTest()
{
    QStringList params;
    //!!! should we test version output? Really we want at least 1.7.x
    //!!! for options such as merge --tool
    params << "--version";
    runner->requestAction(HgAction(ACT_TEST_HG, m_myDirPath, params));
}

void MainWindow::hgTestExtension()
{
    QStringList params;
    params << "--version";
    runner->requestAction(HgAction(ACT_TEST_HG_EXT, m_myDirPath, params));
}

void MainWindow::hgStat()
{
    QStringList params;

    if (showAllFiles) {
        params << "stat" << "-A";
    } else {
        params << "stat" << "-ardum";
    }

    lastStatOutput = "";

    runner->requestAction(HgAction(ACT_STAT, workFolderPath, params));
}

void MainWindow::hgQueryPaths()
{
    // Quickest is to just read the file

    QFileInfo hgrc(workFolderPath + "/.hg/hgrc");

    QString path;

    if (hgrc.exists()) {
        QSettings s(hgrc.canonicalFilePath(), QSettings::IniFormat);
        s.beginGroup("paths");
        path = s.value("default").toString();
    }

    remoteRepoPath = path;

    // We have to do this here, because commandCompleted won't be called
    MultiChoiceDialog::addRecentArgument("local", workFolderPath);
    MultiChoiceDialog::addRecentArgument("remote", remoteRepoPath);
    hgTabs->setWorkFolderAndRepoNames(workFolderPath, remoteRepoPath);
    
    hgQueryBranch();
    return;

/* The classic method!

    QStringList params;
    params << "paths";
    runner->requestAction(HgAction(ACT_QUERY_PATHS, workFolderPath, params));
*/
}

void MainWindow::hgQueryBranch()
{
    // Quickest is to just read the file

    QFile hgbr(workFolderPath + "/.hg/branch");

    QString br = "default";

    if (hgbr.exists() && hgbr.open(QFile::ReadOnly)) {
        QByteArray ba = hgbr.readLine();
        br = QString::fromUtf8(ba).trimmed();
    }
    
    currentBranch = br;
    
    // We have to do this here, because commandCompleted won't be called
    hgStat();
    return;

/* The classic method!

    QStringList params;
    params << "branch";
    runner->requestAction(HgAction(ACT_QUERY_BRANCH, workFolderPath, params));
*/
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
    QStringList allFiles = hgTabs->getAllCommittableFiles();
    QStringList reportFiles = files;
    if (reportFiles.empty()) {
        reportFiles = allFiles;
    }

    QString subsetNote;
    if (reportFiles != allFiles) {
        subsetNote = tr("<p><b>Note:</b> you are committing only the files you have selected, not all of the files that have been changed!");
    }
    
    QString cf(tr("Commit files"));

    if (ConfirmCommentDialog::confirmAndGetLongComment
        (this,
         cf,
         tr("<h3>%1</h3><p>%2%3").arg(cf)
         .arg(tr("You are about to commit the following files."))
         .arg(subsetNote),
         tr("<h3>%1</h3><p>%2%3").arg(cf)
         .arg(tr("You are about to commit %n file(s).", "", reportFiles.size()))
         .arg(subsetNote),
         reportFiles,
         comment,
         tr("Commit"))) {

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
         tag,
         tr("Add Tag"))) {
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

    if (!QDir(workFolderPath).exists()) return;
    QFile f(hgIgnorePath);
    if (!f.exists()) {
        f.open(QFile::WriteOnly);
        QTextStream *ts = new QTextStream(&f);
        *ts << "syntax: glob\n";
        delete ts;
        f.close();
    }
    
    params << hgIgnorePath;
    
    QString editor = getEditorBinaryName();

    if (editor == "") {
        DEBUG << "Failed to find a text editor" << endl;
        //!!! visible error!
        return;
    }

    HgAction action(ACT_HG_IGNORE, workFolderPath, params);
    action.executable = editor;

    runner->requestAction(action);
}

QString MainWindow::getDiffBinaryName()
{
    QSettings settings;
    settings.beginGroup("Locations");
    return settings.value("extdiffbinary", "").toString();
}

QString MainWindow::getMergeBinaryName()
{
    QSettings settings;
    settings.beginGroup("Locations");
    return settings.value("mergebinary", "").toString();
}

QString MainWindow::getEditorBinaryName()
{
    QSettings settings;
    settings.beginGroup("Locations");
    return settings.value("editorbinary", "").toString();
}

void MainWindow::hgShowSummary()
{
    QStringList params;
    
    params << "diff" << "--stat";

    runner->requestAction(HgAction(ACT_DIFF_SUMMARY, workFolderPath, params));
}

void MainWindow::hgFolderDiff()
{
    QString diff = getDiffBinaryName();
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
    QString diff = getDiffBinaryName();
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
    QString diff = getDiffBinaryName();
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
    bool all = false;

    QStringList files = hgTabs->getSelectedRevertableFiles();
    QStringList allFiles = hgTabs->getAllRevertableFiles();
    if (files.empty() || files == allFiles) {
        files = allFiles;
        all = true;
    }
    
    QString subsetNote;
    if (!all) {
        subsetNote = tr("<p><b>Note:</b> you are reverting only the files you have selected, not all of the files that have been changed!");
    }

    QString rf(tr("Revert files"));

    // Set up params before asking for confirmation, because there is
    // a failure case here that we would need to report on early

    DEBUG << "hgRevert: justMerged = " << justMerged << ", mergeTargetRevision = " << mergeTargetRevision << endl;

    if (justMerged) {

        // This is a little fiddly.  The proper way to "revert" the
        // whole of an uncommitted merge is with "hg update --clean ."
        // But if the user has selected only some files, we're sort of
        // promising to revert only those, which means we need to
        // specify which parent to revert to.  We can only do that if
        // we have a record of it, which we do if you just did the
        // merge from within easyhg but don't if you've exited and
        // restarted, or changed repository, since then.  Hmmm.

        if (all) {
            params << "update" << "--clean" << ".";
        } else {
            if (mergeTargetRevision != "") {
                params << "revert" << "--rev"
                       << Changeset::hashOf(mergeTargetRevision)
                       << "--" << files;
            } else {
                QMessageBox::information
                    (this, tr("Unable to revert"),
                     tr("<qt><b>Sorry, unable to revert these files</b><br><br>EasyMercurial can only revert a subset of files during a merge if it still has a record of which parent was the original merge target; that information is no longer available.<br><br>This is a limitation of EasyMercurial.  Consider reverting all files, or using hg revert with a specific revision at the command-line instead.</qt>"));
                return;
            }
        }
    } else {
        params << "revert" << "--" << files;
    }

    if (ConfirmCommentDialog::confirmDangerousFilesAction
        (this,
         rf,
         tr("<h3>%1</h3><p>%2%3").arg(rf)
         .arg(tr("You are about to <b>revert</b> the following files to their previous committed state.<br><br>This will <b>throw away any changes</b> that you have made to these files but have not committed."))
         .arg(subsetNote),
         tr("<h3>%1</h3><p>%2%3").arg(rf)
         .arg(tr("You are about to <b>revert</b> %n file(s).<br><br>This will <b>throw away any changes</b> that you have made to these files but have not committed.", "", files.size()))
         .arg(subsetNote),
         files,
         tr("Revert"))) {

        lastRevertedFiles = files;
        
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

    QString merge = getMergeBinaryName();
    if (merge != "") {
        params << "--tool" << merge;
    }

    QStringList files = hgTabs->getSelectedUnresolvedFiles();
    if (files.empty()) {
        params << "--all";
    } else {
        params << "--" << files;
    }

    if (currentParents.size() == 1) {
        mergeTargetRevision = currentParents[0]->id();
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

    QString merge = getMergeBinaryName();
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

    QString merge = getMergeBinaryName();
    if (merge != "") {
        params << "--tool" << merge;
    }
    
    if (currentParents.size() == 1) {
        mergeTargetRevision = currentParents[0]->id();
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

void MainWindow::hgPull()
{
    if (ConfirmCommentDialog::confirm
        (this, tr("Confirm pull"),
         format3(tr("Confirm pull from remote repository"),
                 tr("You are about to pull changes from the following remote repository:"),
                 remoteRepoPath),
         tr("Pull"))) {

        QStringList params;
        params << "pull" << remoteRepoPath;
        runner->requestAction(HgAction(ACT_PULL, workFolderPath, params));
    }
}

void MainWindow::hgPush()
{
    if (ConfirmCommentDialog::confirm
        (this, tr("Confirm push"),
         format3(tr("Confirm push to remote repository"),
                 tr("You are about to push your changes to the following remote repository:"),
                 remoteRepoPath),
         tr("Push"))) {

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
    if (fsWatcher) {
        delete m_fsWatcherGeneralTimer;
        m_fsWatcherGeneralTimer = 0;
        delete m_fsWatcherRestoreTimer;
        m_fsWatcherRestoreTimer = 0;
        delete fsWatcher;
        fsWatcher = 0;
    }
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

        QSettings settings;
        settings.beginGroup("General");
        QString lastChoice = settings.value("lastopentype", "local").toString();
        if (lastChoice != "local" &&
            lastChoice != "remote" &&
            lastChoice != "init") {
            lastChoice = "local";
        }

        d->setCurrentChoice(lastChoice);

        if (d->exec() == QDialog::Accepted) {

            QString choice = d->getCurrentChoice();
            settings.setValue("lastopentype", choice);

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

        // New block to ensure QSettings is deleted before
        // hgQueryPaths called.  NB use of absoluteFilePath instead of
        // canonicalFilePath, which would fail if the file did not yet
        // exist

        {
            QSettings s(hgrc.absoluteFilePath(), QSettings::IniFormat);
            s.beginGroup("paths");
            s.setValue("default", d->getArgument());
        }

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

bool MainWindow::askAboutUnknownFolder(QString arg)
{    
    bool result = (QMessageBox::question
                   (this, tr("Path does not exist"),
                    tr("<qt><b>Path does not exist: create it?</b><br><br>You asked to open a remote repository by cloning it to \"%1\". This folder does not exist, and neither does its parent.<br><br>Would you like to create the parent folder as well?</qt>").arg(xmlEncode(arg)),
                    QMessageBox::Ok | QMessageBox::Cancel,
                    QMessageBox::Cancel)
                   == QMessageBox::Ok);
    if (result) {
        QDir dir(arg);
        dir.cdUp();
        if (!dir.mkpath(dir.absolutePath())) {
            QMessageBox::critical
                (this, tr("Failed to create folder"),
                 tr("<qt><b>Failed to create folder</b><br><br>Sorry, the path for the parent folder \"%1\" could not be created.</qt>").arg(dir.absolutePath()));
            return false;
        }
        return true;
    }
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
         tr("<qt><b>Local path is in an existing repository</b><br><br>You asked to open a remote repository by cloning it to the local path \"%1\".<br>This path is already inside an existing repository.<br>Please provide a different folder name for the local repository.</qt>").arg(xmlEncode(arg)));
    return false;
}

bool MainWindow::complainAboutCloneToFile(QString arg)
{
    QMessageBox::critical
        (this, tr("Path is a file"),
         tr("<qt><b>Local path is a file</b><br><br>You asked to open a remote repository by cloning it to the local path \"%1\".<br>This path is an existing file.<br>Please provide a new folder name for the local repository.</qt>").arg(xmlEncode(arg)));
    return false;
}

QString MainWindow::complainAboutCloneToExistingFolder(QString arg, QString remote)
{
    // If the directory "arg" exists but "arg" plus the last path
    // component of "remote" does not, then offer the latter as an
    // alternative path

    QString offer;

    QDir d(arg);
    if (d.exists()) {
        if (QRegExp("^\\w+://").indexIn(remote) >= 0) {
            QString rpath = QUrl(remote).path();
            if (rpath != "") {
                rpath = QDir(rpath).dirName();
                if (rpath != "" && !d.exists(rpath)) {
                    offer = d.filePath(rpath);
                }
            }
        }
    }

    if (offer != "") {
        bool result = (QMessageBox::question
                       (this, tr("Folder exists"),
                        tr("<qt><b>Local folder already exists</b><br><br>You asked to open a remote repository by cloning it to \"%1\", but this folder already exists and so cannot be cloned to.<br><br>Would you like to create the new folder \"%2\" instead?</qt>")
                        .arg(xmlEncode(arg)).arg(xmlEncode(offer)),
                        QMessageBox::Ok | QMessageBox::Cancel,
                        QMessageBox::Cancel)
                       == QMessageBox::Ok);
        if (result) return offer;
        else return "";
    }

    QMessageBox::critical
        (this, tr("Folder exists"),
         tr("<qt><b>Local folder already exists</b><br><br>You asked to open a remote repository by cloning it to \"%1\", but this file or folder already exists and so cannot be cloned to.<br>Please provide a different folder name for the local repository.</qt>").arg(xmlEncode(arg)));
    return "";
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
        if (!askAboutUnknownFolder(local)) {
            return false;
        }
    }

    if (status == FolderExists) {
        local = complainAboutCloneToExistingFolder(local, remote);
        if (local == "") return false;
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

    if (settingsDlg->presentationChanged()) {
        hgTabs->updateFileStates();
        updateToolBarStyle();
    }
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
    bool justCreated = false;
    if (!fsWatcher) {
        fsWatcher = new QFileSystemWatcher();
        justCreated = true;
    }

    // QFileSystemWatcher will refuse to add a file or directory to
    // its watch list that it is already watching -- fine, that's what
    // we want -- but it prints a warning when this happens, which is
    // annoying because it would be the normal case for us.  So we'll
    // check for duplicates ourselves.
    QSet<QString> alreadyWatched;
    QStringList dl(fsWatcher->directories());
    foreach (QString d, dl) alreadyWatched.insert(d);
    
    std::deque<QString> pending;
    pending.push_back(workFolderPath);

    while (!pending.empty()) {

        QString path = pending.front();
        pending.pop_front();
        if (!alreadyWatched.contains(path)) {
            fsWatcher->addPath(path);
            DEBUG << "Added to file system watcher: " << path << endl;
        }

        QDir d(path);
        if (d.exists()) {
            d.setFilter(QDir::Dirs | QDir::NoDotAndDotDot |
                        QDir::Readable | QDir::NoSymLinks);
            foreach (QString entry, d.entryList()) {
                if (entry.startsWith('.')) continue;
                QString entryPath = d.absoluteFilePath(entry);
                pending.push_back(entryPath);
            }
        }
    }

    // The general timer isn't really related to the fs watcher
    // object, it just does something similar -- every now and then we
    // do a refresh just to update the history dates etc

    m_fsWatcherGeneralTimer = new QTimer(this);
    connect(m_fsWatcherGeneralTimer, SIGNAL(timeout()),
            this, SLOT(checkFilesystem()));
    m_fsWatcherGeneralTimer->setInterval(30 * 60 * 1000); // half an hour
    m_fsWatcherGeneralTimer->start();

    if (justCreated) {
        connect(fsWatcher, SIGNAL(directoryChanged(QString)),
                this, SLOT(fsDirectoryChanged(QString)));
        connect(fsWatcher, SIGNAL(fileChanged(QString)),
                this, SLOT(fsFileChanged(QString)));
    }
}

void MainWindow::suspendFileSystemWatcher()
{
    DEBUG << "MainWindow::suspendFileSystemWatcher" << endl;
    if (fsWatcher) {
        m_fsWatcherSuspended = true;
        if (m_fsWatcherRestoreTimer) {
            delete m_fsWatcherRestoreTimer;
            m_fsWatcherRestoreTimer = 0;
        }
        m_fsWatcherGeneralTimer->stop();
    }
}

void MainWindow::restoreFileSystemWatcher()
{
    DEBUG << "MainWindow::restoreFileSystemWatcher" << endl;
    if (m_fsWatcherRestoreTimer) delete m_fsWatcherRestoreTimer;
        
    // The restore timer is used to leave a polite interval between
    // being asked to restore the watcher and actually doing so.  It's
    // a single shot timer each time it's used, but we don't use
    // QTimer::singleShot because we want to stop the previous one if
    // it's running (via deleting it)

    m_fsWatcherRestoreTimer = new QTimer(this);
    connect(m_fsWatcherRestoreTimer, SIGNAL(timeout()),
            this, SLOT(actuallyRestoreFileSystemWatcher()));
    m_fsWatcherRestoreTimer->setInterval(1000);
    m_fsWatcherRestoreTimer->setSingleShot(true);
    m_fsWatcherRestoreTimer->start();
}

void MainWindow::actuallyRestoreFileSystemWatcher()
{
    DEBUG << "MainWindow::actuallyRestoreFileSystemWatcher" << endl;
    if (fsWatcher) {
        m_fsWatcherSuspended = false;
        m_fsWatcherGeneralTimer->start();
    }
}

void MainWindow::checkFilesystem()
{
    DEBUG << "MainWindow::checkFilesystem" << endl;
    hgStat();
}

void MainWindow::fsDirectoryChanged(QString d)
{
    DEBUG << "MainWindow::fsDirectoryChanged " << d << endl;
    if (!m_fsWatcherSuspended) {
        hgStat();
    }
}

void MainWindow::fsFileChanged(QString f)
{
    DEBUG << "MainWindow::fsFileChanged " << f << endl;
    if (!m_fsWatcherSuspended) {
        hgStat();
    }
}

QString MainWindow::format3(QString head, QString intro, QString code)
{
    code = xmlEncode(code).replace("\n", "<br>")
#ifndef Q_OS_WIN32
           // The hard hyphen comes out funny on Windows
           .replace("-", "&#8209;")
#endif
           .replace(" ", "&nbsp;");
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

void MainWindow::commandStarting(HgAction action)
{
    // Annoyingly, hg stat actually modifies the working directory --
    // it creates files called hg-checklink and hg-checkexec to test
    // properties of the filesystem.  For safety's sake, suspend the
    // fs watcher while running commands, and restore it shortly after
    // a command has finished.

    suspendFileSystemWatcher();
}

void MainWindow::commandFailed(HgAction action, QString output)
{
    DEBUG << "MainWindow::commandFailed" << endl;
    restoreFileSystemWatcher();

    QString setstr;
#ifdef Q_OS_MAC
    setstr = tr("Preferences");
#else
    setstr = tr("Settings");
#endif

    // Some commands we just have to ignore bad return values from:

    switch(action.action) {
    case ACT_NONE:
        // uh huh
        return;
    case ACT_TEST_HG:
        QMessageBox::warning
            (this, tr("Failed to run Mercurial"),
             format3(tr("Failed to run Mercurial"),
                     tr("The Mercurial program either could not be found or failed to run.<br>Check that the Mercurial program path is correct in %1.<br><br>%2").arg(setstr).arg(output == "" ? QString("") : tr("The test command said:")),
                     output));
        settings();
        return;
    case ACT_TEST_HG_EXT:
        QMessageBox::warning
            (this, tr("Failed to run Mercurial"),
             format3(tr("Failed to run Mercurial with extension enabled"),
                     tr("The Mercurial program failed to run with the EasyMercurial interaction extension enabled.<br>This may indicate an installation problem with EasyMercurial.<br><br>You may be able to continue working if you switch off &ldquo;Use EasyHg Mercurial Extension&rdquo; in %1.  Note that remote repositories that require authentication may not work if you do this.<br><br>%2").arg(setstr).arg(output == "" ? QString("") : tr("The test command said:")),
                     output));
        settings();
        return;
    case ACT_INCOMING:
        // returns non-zero code and no output if the check was
        // successful but there are no changes pending
        if (output.trimmed() == "") {
            showIncoming("");
            return;
        }
        break;
    case ACT_QUERY_HEADS:
        // fails if repo is empty; we don't care (if there's a genuine
        // problem, something else will fail too).  Pretend it
        // succeeded, so that any further actions that are contingent
        // on the success of the heads query get carried out properly.
        commandCompleted(action, "");
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
    case ACT_STAT:
        break; // go on and report
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
    restoreFileSystemWatcher();
    HGACTIONS action = completedAction.action;

    if (action == ACT_NONE) return;

    bool headsChanged = false;
    QStringList oldHeadIds;

    switch (action) {

    case ACT_TEST_HG:
        break;

    case ACT_TEST_HG_EXT:
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
        } else {
            remoteRepoPath = "";
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
        DEBUG << "resolve output = " << output << endl;
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
        QMessageBox::information(this, tr("Clone"), tr("<qt><h3>Clone successful</h3><pre>%1</pre>").arg(xmlEncode(output)));
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
        //!!! use format3?
        QMessageBox::information(this, tr("Merge"), tr("<qt><h3>Merge successful</h3><pre>%1</pre>").arg(xmlEncode(output)));
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
    {
        QSettings settings;
        settings.beginGroup("General");
        if (settings.value("useextension", true).toBool()) {
            hgTestExtension();
        } else if (workFolderPath == "") {
            open();
        } else {
            hgQueryPaths();
        }
        break;
    }
        
    case ACT_TEST_HG_EXT:
        if (workFolderPath == "") {
            open();
        } else{
            hgQueryPaths();
        }
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
        break;

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

    QString dirname = QDir(workFolderPath).dirName();
    if (dirname != "") {
        setWindowTitle(tr("EasyMercurial: %1").arg(dirname));
    } else {
        setWindowTitle(tr("EasyMercurial"));
    }

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

    if (workFolderPath == "" || !workFolderDir.exists(workFolderPath)) {
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
    bool noWorkingCopy = false;
    bool newBranch = false;
    int currentBranchHeads = 0;

    if (currentParents.size() == 1) {
        bool parentIsHead = false;
        Changeset *parent = currentParents[0];
        foreach (Changeset *head, currentHeads) {
            DEBUG << "head branch " << head->branch() << ", current branch " << currentBranch << endl;
            if (head->isOnBranch(currentBranch)) {
                ++currentBranchHeads;
            }
            if (parent->id() == head->id()) {
                parentIsHead = true;
            }
        }
        if (currentBranchHeads == 2 && parentIsHead) {
            canMerge = true;
        }
        if (currentBranchHeads == 0 && parentIsHead) {
            // Just created a new branch
            newBranch = true;
        }
        if (!parentIsHead) {
            canUpdate = true;
            DEBUG << "parent id = " << parent->id() << endl;
            DEBUG << " head ids "<<endl;
            foreach (Changeset *h, currentHeads) {
                DEBUG << "head id = " << h->id() << endl;
            }
        }
        justMerged = false;
    } else if (currentParents.size() == 0) {
        if (currentHeads.size() == 0) {
            // No heads -> empty repo
            emptyRepo = true;
        } else {
            // Heads, but no parents -> no working copy, e.g. we have
            // just converted this repo but haven't updated in it yet.
            // Uncommon but confusing; probably merits a special case
            noWorkingCopy = true;
            canUpdate = true;
        }
        justMerged = false;
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
        if (workFolderPath == "") {
            hgTabs->setState(tr("No repository open"));
        } else {
            hgTabs->setState(tr("(Examining repository)"));
        }
    } else if (emptyRepo) {
        hgTabs->setState(tr("Nothing committed to this repository yet"));
    } else if (noWorkingCopy) {
        hgTabs->setState(tr("No working copy yet: consider updating"));
    } else if (canMerge) {
        hgTabs->setState(tr("<b>Awaiting merge</b> on %1").arg(branchText));
    } else if (!hgTabs->getAllUnresolvedFiles().empty()) {
        hgTabs->setState(tr("Have unresolved files following merge on %1").arg(branchText));
    } else if (haveMerge) {
        hgTabs->setState(tr("Have merged but not yet committed on %1").arg(branchText));
    } else if (newBranch) {
        hgTabs->setState(tr("On %1.  New branch: has not yet been committed").arg(branchText));
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
    openAct -> setStatusTip(tr("Open an existing repository or working folder"));

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
    QShortcut *clearSelectionsShortcut = new QShortcut(Qt::Key_Escape, this);
    connect(clearSelectionsShortcut, SIGNAL(activated()),
            this, SLOT(clearSelections()));
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
    advancedMenu -> addSeparator();
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

    updateToolBarStyle();
}

void MainWindow::updateToolBarStyle()
{
    QSettings settings;
    settings.beginGroup("Presentation");
    bool showText = settings.value("showiconlabels", true).toBool();
    settings.endGroup();
    
    foreach (QToolButton *tb, findChildren<QToolButton *>()) {
        tb->setToolButtonStyle(showText ?
                               Qt::ToolButtonTextUnderIcon :
                               Qt::ToolButtonIconOnly);
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




