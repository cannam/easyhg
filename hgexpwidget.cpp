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

#include "hgexpwidget.h"
#include "common.h"
#include "logparser.h"
#include "changeset.h"
#include "changesetitem.h"
#include "grapher.h"
#include "panner.h"
#include "panned.h"
#include "filestatuswidget.h"

#include <QClipboard>
#include <QContextMenuEvent>
#include <QApplication>

#include <iostream>

HgExpWidget::HgExpWidget(QWidget *parent,
                         QString remoteRepo,
                         QString workFolderPath) :
    QTabWidget(parent)
{
    // Work page
    fileStatusWidget = new FileStatusWidget;
    fileStatusWidget->setLocalPath(workFolderPath);
    fileStatusWidget->setRemoteURL(remoteRepo);
    connect(fileStatusWidget, SIGNAL(selectionChanged()),
            this, SIGNAL(selectionChanged()));
    addTab(fileStatusWidget, tr("My work"));

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
    addTab(historyGraphPageWidget, tr("History"));
}

void HgExpWidget::clearSelections()
{
    fileStatusWidget->clearSelections();
}

bool HgExpWidget::canCommit() const
{
    return fileStatusWidget->haveChangesToCommit();
}

bool HgExpWidget::canAdd() const
{
    if (fileStatusWidget->getSelectedAddableFiles().empty()) return false;
    if (!fileStatusWidget->getSelectedCommittableFiles().empty()) return false;
    if (!fileStatusWidget->getSelectedRemovableFiles().empty()) return false;
    return true;
}

bool HgExpWidget::canRemove() const
{
    if (fileStatusWidget->getSelectedRemovableFiles().empty()) return false;
    if (!fileStatusWidget->getSelectedAddableFiles().empty()) return false;
    return true;
}

bool HgExpWidget::canDoDiff() const
{
    return canCommit();
}

QStringList HgExpWidget::getAllSelectedFiles() const
{
    return fileStatusWidget->getAllSelectedFiles();
}

QStringList HgExpWidget::getSelectedCommittableFiles() const
{
    return fileStatusWidget->getSelectedCommittableFiles();
}

QStringList HgExpWidget::getSelectedAddableFiles() const
{
    return fileStatusWidget->getSelectedAddableFiles();
}

QStringList HgExpWidget::getSelectedRemovableFiles() const
{
    return fileStatusWidget->getSelectedRemovableFiles();
}

void HgExpWidget::updateWorkFolderFileList(QString fileList)
{
    fileStates.parseStates(fileList);
    fileStatusWidget->setFileStates(fileStates);
}

void HgExpWidget::updateLocalRepoHgLogList(QString hgLogList)
{
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
    QGraphicsScene *oldScene = panned->scene();
    panned->setScene(scene);
    panner->setScene(scene);
    if (oldScene) delete oldScene;
    ChangesetItem *tipItem = g.getItemFor(csets[0]);
    if (tipItem) tipItem->ensureVisible();
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

void HgExpWidget::setWorkFolderAndRepoNames(QString workFolderPath, QString remoteRepoPath)
{
    fileStatusWidget->setLocalPath(workFolderPath);
    fileStatusWidget->setRemoteURL(remoteRepoPath);
}
