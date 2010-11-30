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

#include "historywidget.h"

#include "changesetscene.h"
#include "panned.h"
#include "panner.h"
#include "grapher.h"
#include "debug.h"
#include "uncommitteditem.h"

#include <iostream>

#include <QGridLayout>

HistoryWidget::HistoryWidget()
{
    m_panned = new Panned;
    m_panner = new Panner;
    m_uncommitted = new UncommittedItem();
    m_uncommitted->setRow(-1);
    m_uncommittedVisible = false;

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
    if (!m_uncommittedVisible) delete m_uncommitted;
}

void HistoryWidget::clearChangesets()
{
    foreach (Changeset *cs, m_changesets) delete cs;
    m_changesets.clear();
}

void HistoryWidget::setCurrent(QStringList ids)
{
    if (m_currentIds == ids) return;
    DEBUG << "HistoryWidget::setCurrent: " << ids.size() << " ids" << endl;
    m_currentIds.clear();
    foreach (QString id, ids) {
        m_currentIds.push_back(id);
    }
    updateNewAndCurrentItems();
}

void HistoryWidget::showUncommittedChanges(bool show)
{
    m_uncommittedVisible = show;
    layoutAll();
}
    
void HistoryWidget::parseNewLog(QString log)
{
    DEBUG << "HistoryWidget::parseNewLog: log has " << log.length() << " chars" << endl;
    Changesets csets = Changeset::parseChangesets(log);
    DEBUG << "HistoryWidget::parseNewLog: log has " << csets.size() << " changesets" << endl;
    replaceChangesets(csets);
    layoutAll();
}
    
void HistoryWidget::parseIncrementalLog(QString log)
{
    DEBUG << "HistoryWidget::parseIncrementalLog: log has " << log.length() << " chars" << endl;
    Changesets csets = Changeset::parseChangesets(log);
    DEBUG << "HistoryWidget::parseIncrementalLog: log has " << csets.size() << " changesets" << endl;
    if (!csets.empty()) {
        addChangesets(csets);
        layoutAll();
    }
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

    DEBUG << "addChangesets: " << csets.size() << " new changesets" << endl;

    csets << m_changesets;
    m_changesets = csets;
}

void HistoryWidget::layoutAll()
{
    setChangesetParents();

    ChangesetScene *scene = new ChangesetScene();
    ChangesetItem *tipItem = 0;

    QGraphicsScene *oldScene = m_panned->scene();

    // detach m_uncommitted from old scene so it doesn't get deleted
    if (oldScene && (m_uncommitted->scene() == oldScene)) {
        oldScene->removeItem(m_uncommitted);
    }

    m_panned->setScene(0);
    m_panner->setScene(0);

    delete oldScene;

    if (!m_changesets.empty()) {
	Grapher g(scene);
	try {
	    g.layout(m_changesets);
	} catch (std::string s) {
	    std::cerr << "Internal error: Layout failed: " << s << std::endl;
	}
	tipItem = g.getItemFor(m_changesets[0]);
        DEBUG << "tipItem is " << tipItem << " for tip changeset " 
              << m_changesets[0]->id() << endl;
    }

    if (m_uncommittedVisible) {
        scene->addItem(m_uncommitted);
    }

    m_panned->setScene(scene);
    m_panner->setScene(scene);

    updateNewAndCurrentItems();

    if (m_uncommittedVisible) {
        m_uncommitted->ensureVisible();
    } else if (tipItem) {
        DEBUG << "asking tip item to be visible" << endl;
        tipItem->ensureVisible();
    }
}

void HistoryWidget::setChangesetParents()
{
    for (int i = 0; i+1 < m_changesets.size(); ++i) {
        Changeset *cs = m_changesets[i];
        // Need to reset this, as Grapher::layout will recalculate it
        // and we don't want to end up with twice the children for
        // each parent...
        cs->setChildren(QStringList());
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
        
        csit->setCurrent(current);
        csit->setNew(newid);
        
        if (current) {
            m_uncommitted->setRow(csit->row() - 1);
            m_uncommitted->setColumn(csit->column());
            m_uncommitted->setWide(csit->isWide());
            m_uncommitted->setBranch(csit->getChangeset()->branch());
        }
    }
}
