/****************************************************************************
** Copyright (C) Jari Korhonen, 2010 (under lgpl)
****************************************************************************/

#include "hgexpwidget.h"
#include "common.h"
#include "logparser.h"
#include "changeset.h"
#include "changesetitem.h"
#include "grapher.h"
#include "panner.h"
#include "panned.h"

#include <QClipboard>
#include <QContextMenuEvent>
#include <QApplication>

#include <iostream>

#define REMOTE_REPO_STR  "Remote repository: "
#define LOCAL_REPO_STR   "Local repository: "
#define WORKFOLDER_STR   "Working folder: "


const char hgStatViewOptions[NUM_STAT_FILE_TYPES] = {'m','a','r','d','u','c','i'};

const char *statFilesStr[NUM_STAT_FILE_TYPES] =  {  "M: Modified",
                                                    "A: To be added on next commit",
                                                    "R: To be removed on next commit",
                                                    "!: Tracked, locally deleted",
                                                    "?: Unknown, not yet tracked",
                                                    "C: Clean (not changed)",
                                                    "I: Ignored (via .hgignore file)"};


HgExpWidget::HgExpWidget(QWidget *parent, QString remoteRepo, QString workFolderPath, unsigned char viewFileTypesBits): QTabWidget(parent)
{
    //Work page
    //Work page
    //Work page

    //Remote repo
    grpRemoteRepo = new QGroupBox(tr(REMOTE_REPO_STR) + remoteRepo);
    grpRemoteRepo -> setMinimumHeight(24);

    //Local Repo
    grpLocalRepo = new QGroupBox(tr(LOCAL_REPO_STR) + workFolderPath + getHgDirName());
    parentsLabel = new QLabel(tr("Working folder parent(s):"));
    localRepoHgParentsList = new QListWidget;
    localRepoHgParentsList -> setSelectionMode(QAbstractItemView::NoSelection);
    parentsLayout = new QVBoxLayout;
    parentsLayout -> addWidget(parentsLabel);
    parentsLayout -> addWidget(localRepoHgParentsList);
    grpLocalRepo -> setLayout(parentsLayout);
    copyCommentAct = new QAction("Copy comment", localRepoHgParentsList);
    userListMenu = new QMenu(localRepoHgParentsList);
    userListMenu -> addAction(copyCommentAct);
    connect(copyCommentAct, SIGNAL(triggered()), this, SLOT(copyComment()));

    //Workfolder
    grpWorkFolder = new QGroupBox(tr(WORKFOLDER_STR) + workFolderPath);
    workFolderLayout = new QHBoxLayout;
    workFolderFileList = new QListWidget;
    workFolderFileList -> setSelectionMode(QAbstractItemView::ExtendedSelection);
    grpViewFileTypes = new QGroupBox;
    fileTypesLayout = new QVBoxLayout;

    for(int i = 0; i < NUM_STAT_FILE_TYPES; i++)
    {
        chkViewFileTypes[i] = new QCheckBox(statFilesStr[i]);
        if ((1U << i) & viewFileTypesBits)
        {
            chkViewFileTypes[i]->setChecked(true);
        }
        else
        {
            chkViewFileTypes[i]->setChecked(false);
        }
        connect(chkViewFileTypes[i], SIGNAL(stateChanged(int)), this, SIGNAL(workFolderViewTypesChanged()));
        fileTypesLayout -> addWidget(chkViewFileTypes[i]);
    }

    grpViewFileTypes -> setLayout(fileTypesLayout);
    workFolderLayout->addWidget(workFolderFileList, 3);
    workFolderLayout->addWidget(grpViewFileTypes, 1);
    grpWorkFolder -> setLayout(workFolderLayout);

    workPageWidget = new QWidget;
    mainLayout = new QVBoxLayout(workPageWidget);
    mainLayout -> addWidget(grpRemoteRepo, 1);
    mainLayout -> addWidget(grpLocalRepo, 8);
    mainLayout -> addWidget(grpWorkFolder, 12);
    addTab(workPageWidget, tr("Work"));

    // History graph page
    historyGraphPageWidget = new QWidget;
    Panned *panned = new Panned;
    Panner *panner = new Panner;
    historyGraphWidget = panned;
    historyGraphPanner = panner;
    QGridLayout *layout = new QGridLayout;
    layout->addWidget(historyGraphWidget, 0, 0);
    layout->addWidget(historyGraphPanner, 0, 1);
    panner->setMaximumWidth(80);
    panner->connectToPanned(panned);
    historyGraphPageWidget->setLayout(layout);
    addTab(historyGraphPageWidget, tr("History (graph)"));


    //History page
    //History page
    //History page
    historyPageWidget = new QWidget;
    localRepoHgLogList = new QListWidget;
    localRepoHgLogList->setFont(QFont("Courier New"));
    localRepoHgLogList -> setSelectionMode(QAbstractItemView::ExtendedSelection);

    historyLayout = new QVBoxLayout(historyPageWidget);
    historyLayout->addWidget(localRepoHgLogList);
    addTab(historyPageWidget, tr("History (log)"));

    //Heads page
    //Heads page
    //Heads page
    headsPageWidget = new QWidget;
    localRepoHeadsList = new QListWidget;
    localRepoHeadsList -> setSelectionMode(QAbstractItemView::ExtendedSelection);

    headsLayout = new QVBoxLayout(headsPageWidget);
    headsLayout->addWidget(localRepoHeadsList);
    addTab(headsPageWidget, tr("Heads"));

    //Initially, only work page is active
    setTabEnabled(HEADSTAB, false);
    setTabEnabled(HISTORYTAB, false);
}

void HgExpWidget::contextMenuEvent(QContextMenuEvent * event)
{
    if (copyCommentAct -> isEnabled())
    {
        QPoint topL;
        QPoint bottomR;

        topL = localRepoHgParentsList->
            mapToGlobal(QPoint(0, 0));
        bottomR = localRepoHgParentsList->
            mapToGlobal(QPoint(localRepoHgParentsList -> width(), localRepoHgParentsList -> height()));

        if ((event -> globalPos().x() > topL.x()) && (event -> globalPos().x() < bottomR.x()))
        {
            if ((event -> globalPos().y() > topL.y()) && (event -> globalPos().y() < bottomR.y()))
            {
                userListMenu->exec(event -> globalPos());
            }
        }
    }
}

void HgExpWidget::copyComment()
{
    if (localRepoHgParentsList -> count() >= 1)
    {
        QListWidgetItem *it =  localRepoHgParentsList -> item(0);
        QString tmp = it -> text();
        int ind = tmp.indexOf("summary:");
        if (ind != -1)
        {
            QString comment;
            ind += 11;   //jump over word "summary:"

            comment = tmp.mid(ind);

            QClipboard *clipboard = QApplication::clipboard();
            clipboard->setText(comment);
        }
    }
}



QString HgExpWidget::getStatFlags()
{
    QString ret;

    for(int i = 0; i < NUM_STAT_FILE_TYPES; i++)
    {
        if (Qt::Checked == chkViewFileTypes[i]->checkState())
        {
            ret += hgStatViewOptions[i];
        }
    }

    return ret;
}


unsigned char HgExpWidget::getFileTypesBits()
{
    unsigned char ret;

    ret = 0;

    for(int i = 0; i < NUM_STAT_FILE_TYPES; i++)
    {
        if (Qt::Checked == chkViewFileTypes[i]->checkState())
        {
            ret |= (1U << i);
        }
    }

    return ret;
}


void HgExpWidget::updateWorkFolderFileList(QString fileList)
{
    workFolderFileList-> clear();
    workFolderFileList -> addItems(fileList.split("\n"));
}

void HgExpWidget::updateLocalRepoHeadsList(QString headList)
{
    localRepoHeadsList-> clear();
    localRepoHeadsList -> addItems(splitChangeSets(headList));

    //heads list is interesting only when we have 2 or more
    if (localRepoHeadsList-> count() < 2)
    {
        setTabEnabled(HEADSTAB, false);
    }
    else
    {
        setTabEnabled(HEADSTAB, true);
    }
}


void HgExpWidget::clearLists()
{
    localRepoHeadsList-> clear();
    localRepoHgParentsList-> clear();
    workFolderFileList-> clear();
    localRepoHgLogList -> clear();
}

void HgExpWidget::updateLocalRepoParentsList(QString parentsList)
{
    localRepoHgParentsList-> clear();
    localRepoHgParentsList -> addItems(splitChangeSets(parentsList));
}

void HgExpWidget::updateLocalRepoHgLogList(QString hgLogList)
{
    localRepoHgLogList -> clear();
    localRepoHgLogList -> addItems(splitChangeSets(hgLogList));

    //!!!
    Panned *panned = static_cast<Panned *>(historyGraphWidget);
    Panner *panner = static_cast<Panner *>(historyGraphPanner);
    QGraphicsScene *scene = new QGraphicsScene();
    Changesets csets = parseChangeSets(hgLogList);
    if (csets.empty()) return;
    Grapher g(scene);
    try {
	g.layout(csets);
    } catch (std::string s) {
	std::cerr << "Internal error: Layout failed: " << s << std::endl;
    }
    panned->scene()->deleteLater();
    panned->setScene(scene);
    panner->scene()->deleteLater();
    panner->setScene(scene);
    ChangesetItem *tipItem = g.getItemFor(csets[0]);
    if (tipItem) tipItem->ensureVisible();
}



int HgExpWidget::findLineStart(int nowIndex, QString str)
{
    if (nowIndex < 0)
    {
        return -1;
    }

    while(str.at(nowIndex) != '\n')
    {
        if (nowIndex == 0)
        {
            return nowIndex;
        }
        nowIndex--;
    }
    return nowIndex + 1;
}


QStringList HgExpWidget::splitChangeSets(QString chgSetsStr)
{
    return LogParser(chgSetsStr).split();
    /*
    int currChgSet;
    int currChgSetLineStart;

    int prevChgSet;
    QStringList tmp;

    currChgSet = chgSetsStr.indexOf(CHGSET);
    currChgSetLineStart = findLineStart(currChgSet, chgSetsStr);
    prevChgSet = -1;
    while (currChgSet != -1)
    {
        if (prevChgSet != -1)
        {
            tmp.append(chgSetsStr.mid(prevChgSet, (currChgSetLineStart - prevChgSet - 1)));
        }

        prevChgSet = currChgSetLineStart;

        currChgSet = chgSetsStr.indexOf(CHGSET, currChgSet + 1);
        currChgSetLineStart = findLineStart(currChgSet, chgSetsStr);
    }

    if (prevChgSet != -1)
    {
        //Last changeset
        tmp.append(chgSetsStr.mid(prevChgSet));
    }
    else
    {
        //Only changeset (if any)
        if (!chgSetsStr.isEmpty())
        {
            tmp.append(chgSetsStr.mid(0));
        }
    }

    return tmp;
    */
}

Changesets HgExpWidget::parseChangeSets(QString changeSetsStr)
{
    Changesets csets;
    LogList log = LogParser(changeSetsStr).parse();
    foreach (LogEntry e, log) {
        Changeset *cs = new Changeset();
        foreach (QString key, e.keys()) {
	    if (key == "parents") {
		QStringList parents = e.value(key).split
		    (" ", QString::SkipEmptyParts);
		cs->setParents(parents);
	    } else if (key == "timestamp") {
		cs->setTimestamp(e.value(key).split(" ")[0].toULongLong());
	    } else {
		cs->setProperty(key.toLocal8Bit().data(), e.value(key));
	    }
        }
        csets.push_back(cs);
    }
    for (int i = 0; i+1 < csets.size(); ++i) {
	Changeset *cs = csets[i];
	if (cs->parents().empty()) {
	    QStringList list;
	    list.push_back(csets[i+1]->id());
	    cs->setParents(list);
	}
    }
    return csets;
}

QString HgExpWidget::getCurrentFileListLine()
{
    if (workFolderFileList -> currentItem() != NULL)
    {
        return workFolderFileList -> currentItem()->text();
    }
    return "";
}

void HgExpWidget::setWorkFolderAndRepoNames(QString workFolderPath, QString remoteRepoPath)
{
    grpRemoteRepo -> setTitle(tr(REMOTE_REPO_STR) + remoteRepoPath);
    grpLocalRepo -> setTitle(tr(LOCAL_REPO_STR) + workFolderPath + getHgDirName());
    grpWorkFolder -> setTitle(tr(WORKFOLDER_STR) + workFolderPath);
}

#define MERC_SHA1_MARKER_LEN 12
QString HgExpWidget::findRev(QString itemText, QString & smallRev)
{
    QString tmp(itemText);
    int i;
    int j;

    smallRev ="0";

    i = tmp.indexOf(CHGSET);
    if (i != -1)
    {
        j = i + 10;
        i = tmp.indexOf(":", j);  //xx:yyyyyy after changeset:

        if (i != -1)
        {
            smallRev = tmp.mid(j, (i-j));
            return tmp.mid(i+1, MERC_SHA1_MARKER_LEN);
        }
    }

    return "";
}

void HgExpWidget::getHistoryDiffRevisions(QString& revA, QString& revB)
{
    QList <QListWidgetItem *> histList = localRepoHgLogList->selectedItems();
    QList <QListWidgetItem *> headList = localRepoHeadsList->selectedItems();

    QString revATmp;
    QString revBTmp;
    QString smallRevA;
    QString smallRevB;
    QString txtA;
    QString txtB;

    if (histList.count() == REQUIRED_CHGSET_DIFF_COUNT)
    {
        txtA = histList.last()->text();
        txtB = histList.first()->text();

    }
    else if (headList.count() == REQUIRED_CHGSET_DIFF_COUNT)
    {
        txtA = headList.last()->text();
        txtB = headList.first()->text();
    }
    else
    {
        revA = "";
        revB = "";
        return;
    }

    revATmp = findRev(txtA, smallRevA);
    revBTmp = findRev(txtB, smallRevB);

    //Switch order according to repo small revision number (user can select items from list in "wrong" order)
    if (smallRevB.toULongLong() > smallRevA.toULongLong())
    {
        revA = revATmp;
        revB = revBTmp;
    }
    else
    {
        revA = revBTmp;
        revB = revATmp;
    }
}


void HgExpWidget::getUpdateToRevRevision(QString& rev)
{
    QList <QListWidgetItem *> histList = localRepoHgLogList->selectedItems();
    QString txt;
    QString smallRev;


    if (histList.count() == 1)
    {
        txt = histList.first()->text();
        rev = findRev(txt, smallRev);
    }
    else
    {
        rev = "";
    }
}


void HgExpWidget::enableDisableOtherTabs(int tabPage)
{
    static int oldTabPage = -1;

    if  (tabPage != oldTabPage)
    {
        oldTabPage = tabPage;
        if (tabPage == WORKTAB)
        {
            copyCommentAct -> setEnabled(true);
        }
        else
        {
            copyCommentAct -> setEnabled(false);
        }
    }

    //history list is only interesting when we have something in it ;-)
    if (localRepoHgLogList -> count() < 2)
    {
        setTabEnabled(HISTORYTAB, false);
    }
    else
    {
        setTabEnabled(HISTORYTAB, true);
    }

    //history list is only interesting when we have something in it ;-)
    if (localRepoHgLogList -> count() < 2)
    {
        setTabEnabled(HISTORYTAB, false);
    }
    else
    {
        setTabEnabled(HISTORYTAB, true);
    }
}




