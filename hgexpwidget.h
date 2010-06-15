#ifndef HGEXPWIDGET_H
#define HGEXPWIDGET_H

/****************************************************************************
** Copyright (C) Jari Korhonen, 2010 (under lgpl)
****************************************************************************/

#include <QtGui>
#include <QtCore>
#include <QMenu>

#include "common.h"

#define NUM_STAT_FILE_TYPES 7


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
    QGroupBox   *grpRemoteRepo;
    QWidget     *workPageWidget;
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

    QString     findRev(QString itemText, QString& smallRev);
    QStringList splitChangeSets(QString chgSetsStr);
    int findLineStart(int nowIndex, QString chgSetsStr);
    void contextMenuEvent (QContextMenuEvent * event);
};

#endif // HGEXPWIDGET_H
