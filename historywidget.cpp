/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    EasyMercurial

    Based on HgExplorer by Jari Korhonen
    Copyright (c) 2010 Jari Korhonen
    Copyright (c) 2011 Chris Cannam
    Copyright (c) 2011 Queen Mary, University of London
    
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "historywidget.h"

#include "changesetscene.h"
#include "panned.h"
#include "panner.h"
#include "grapher.h"
#include "debug.h"
#include "uncommitteditem.h"

#include <iostream>

#include <QGridLayout>

HistoryWidget::HistoryWidget() :
    m_showUncommitted(false),
    m_refreshNeeded(false)
{
    m_panned = new Panned;
    m_panner = new Panner;

    m_panned->setDragMode(QGraphicsView::ScrollHandDrag);
    m_panned->setViewportUpdateMode(QGraphicsView::BoundingRectViewportUpdate);

    QGridLayout *layout = new QGridLayout;
    layout->addWidget(m_panned, 0, 0);
    layout->addWidget(m_panner, 0, 1);
    m_panner->setMaximumWidth(80);
    m_panner->connectToPanned(m_panned);

    setLayout(layout);
}

HistoryWidget::~HistoryWidget()
{
    clearChangesets();
}

QGraphicsScene *HistoryWidget::scene()
{
    return m_panned->scene();
}

void HistoryWidget::clearChangesets()
{
    foreach (Changeset *cs, m_changesets) delete cs;
    m_changesets.clear();
}

void HistoryWidget::setCurrent(QStringList ids, QString branch,
                               bool showUncommitted)
{
    if (m_currentIds == ids &&
        m_currentBranch == branch &&
        m_showUncommitted == showUncommitted) return;

    DEBUG << "HistoryWidget::setCurrent: " << ids.size() << " ids, "
          << "showUncommitted: " << showUncommitted << endl;

    m_currentIds.clear();
    m_currentBranch = branch;
    m_showUncommitted = showUncommitted;

    if (ids.empty()) return;

    foreach (QString id, ids) {
        m_currentIds.push_back(id);
    }

    m_refreshNeeded = true;
}
    
void HistoryWidget::parseNewLog(QString log)
{
    DEBUG << "HistoryWidget::parseNewLog: log has " << log.length() << " chars" << endl;
    Changesets csets = Changeset::parseChangesets(log);
    DEBUG << "HistoryWidget::parseNewLog: log has " << csets.size() << " changesets" << endl;
    replaceChangesets(csets);
    m_refreshNeeded = true;
}
    
void HistoryWidget::parseIncrementalLog(QString log)
{
    DEBUG << "HistoryWidget::parseIncrementalLog: log has " << log.length() << " chars" << endl;
    Changesets csets = Changeset::parseChangesets(log);
    DEBUG << "HistoryWidget::parseIncrementalLog: log has " << csets.size() << " changesets" << endl;
    if (!csets.empty()) {
        addChangesets(csets);
    }
    m_refreshNeeded = true;
}

void HistoryWidget::replaceChangesets(Changesets csets)
{
    QSet<QString> oldIds;
    foreach (Changeset *cs, m_changesets) {
        oldIds.insert(cs->id());
    }

    QSet<QString> newIds;
    foreach (Changeset *cs, csets) {
        if (!oldIds.contains(cs->id())) {
            newIds.insert(cs->id());
        }
    }

    if (newIds.size() == csets.size()) {
        // completely new set, unrelated to the old: don't mark new
        m_newIds.clear();
    } else {
        m_newIds = newIds;
    }

    clearChangesets();
    m_changesets = csets;
}

void HistoryWidget::addChangesets(Changesets csets)
{
    m_newIds.clear();

    if (csets.empty()) return;

    foreach (Changeset *cs, csets) {
        m_newIds.insert(cs->id());
    }

    DEBUG << "addChangesets: " << csets.size() << " new changesets have ("
          << m_changesets.size() << " already)" << endl;

    csets << m_changesets;
    m_changesets = csets;
}

void HistoryWidget::update()
{
    if (m_refreshNeeded) {
        layoutAll();
    }
}

void HistoryWidget::layoutAll()
{
    m_refreshNeeded = false;

    setChangesetParents();

    ChangesetScene *scene = new ChangesetScene();
    ChangesetItem *tipItem = 0;

    QGraphicsScene *oldScene = m_panned->scene();

    m_panned->setScene(0);
    m_panner->setScene(0);

    delete oldScene;

    QGraphicsItem *toFocus = 0;

    if (!m_changesets.empty()) {
	Grapher g(scene);
	try {
	    g.layout(m_changesets,
                     m_showUncommitted ? m_currentIds : QStringList(),
                     m_currentBranch);
	} catch (std::string s) {
	    std::cerr << "Internal error: Layout failed: " << s << std::endl;
	}
        toFocus = g.getUncommittedItem();
        if (!toFocus) {
            toFocus = g.getItemFor(m_changesets[0]);
        }
    }

    m_panned->setScene(scene);
    m_panner->setScene(scene);

    updateNewAndCurrentItems();

    if (toFocus) {
        toFocus->ensureVisible();
    }

    connectSceneSignals();
}

void HistoryWidget::setChangesetParents()
{
    for (int i = 0; i < m_changesets.size(); ++i) {
        Changeset *cs = m_changesets[i];
        // Need to reset this, as Grapher::layout will recalculate it
        // and we don't want to end up with twice the children for
        // each parent...
        cs->setChildren(QStringList());
    }
    for (int i = 0; i+1 < m_changesets.size(); ++i) {
        Changeset *cs = m_changesets[i];
        if (cs->parents().empty()) {
            QStringList list;
            list.push_back(m_changesets[i+1]->id());
            cs->setParents(list);
        }
    }
}

void HistoryWidget::updateNewAndCurrentItems()
{
    QGraphicsScene *scene = m_panned->scene();
    if (!scene) return;

    QList<QGraphicsItem *> items = scene->items();
    foreach (QGraphicsItem *it, items) {

        ChangesetItem *csit = dynamic_cast<ChangesetItem *>(it);
        if (!csit) continue;

        QString id = csit->getChangeset()->id();

        bool current = m_currentIds.contains(id);
        if (current) {
            DEBUG << "id " << id << " is current" << endl;
        }
        bool newid = m_newIds.contains(id);
        if (newid) {
            DEBUG << "id " << id << " is new" << endl;
        }

        if (csit->isCurrent() != current || csit->isNew() != newid) {
            csit->setCurrent(current);
            csit->setNew(newid);
            csit->update();
        }
    }
}

void HistoryWidget::connectSceneSignals()
{
    ChangesetScene *scene = qobject_cast<ChangesetScene *>(m_panned->scene());
    if (!scene) return;
    
    connect(scene, SIGNAL(commit()),
            this, SIGNAL(commit()));
    
    connect(scene, SIGNAL(revert()),
            this, SIGNAL(revert()));
    
    connect(scene, SIGNAL(diffWorkingFolder()),
            this, SIGNAL(diffWorkingFolder()));

    connect(scene, SIGNAL(showSummary()),
            this, SIGNAL(showSummary()));

    connect(scene, SIGNAL(showWork()),
            this, SIGNAL(showWork()));
    
    connect(scene, SIGNAL(updateTo(QString)),
            this, SIGNAL(updateTo(QString)));

    connect(scene, SIGNAL(diffToCurrent(QString)),
            this, SIGNAL(diffToCurrent(QString)));

    connect(scene, SIGNAL(diffToParent(QString, QString)),
            this, SIGNAL(diffToParent(QString, QString)));

    connect(scene, SIGNAL(showSummary(Changeset *)),
            this, SIGNAL(showSummary(Changeset *)));

    connect(scene, SIGNAL(mergeFrom(QString)),
            this, SIGNAL(mergeFrom(QString)));

    connect(scene, SIGNAL(newBranch(QString)),
            this, SIGNAL(newBranch(QString)));

    connect(scene, SIGNAL(tag(QString)),
            this, SIGNAL(tag(QString)));
}
