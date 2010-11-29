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

#include <iostream>

#include <QGridLayout>

HistoryWidget::HistoryWidget()
{
    m_panned = new Panned;
    m_panner = new Panner;

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

void HistoryWidget::clearChangesets()
{
    foreach (Changeset *cs, m_changesets) delete cs;
    m_changesets.clear();
}

void HistoryWidget::setCurrent(QStringList ids)
{
    if (m_currentIds == ids) return;
    DEBUG << "HistoryWidget::setCurrent: " << ids.size() << " ids" << endl;
    m_currentIds = ids;
    updateCurrentItems();
}

void HistoryWidget::showUncommittedChanges(bool show)
{
    //!!! implement!
}
    
void HistoryWidget::parseNewLog(QString log)
{
    DEBUG << "HistoryWidget::parseNewLog: log has " << log.length() << " chars" << endl;
    Changesets csets = Changeset::parseChangesets(log);
    DEBUG << "HistoryWidget::parseNewLog: log has " << csets.size() << " changesets" << endl;
    clearChangesets();
    m_changesets = csets;
    layoutAll();
}
    
void HistoryWidget::parseIncrementalLog(QString log)
{
    DEBUG << "HistoryWidget::parseIncrementalLog: log has " << log.length() << " chars" << endl;
    Changesets csets = Changeset::parseChangesets(log);
    DEBUG << "HistoryWidget::parseIncrementalLog: log has " << csets.size() << " changesets" << endl;
    if (!csets.empty()) {
        csets << m_changesets;
        m_changesets = csets;
        layoutAll();
    }
}

void HistoryWidget::layoutAll()
{
    setChangesetParents();

    ChangesetScene *scene = new ChangesetScene();
    ChangesetItem *tipItem = 0;

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

    QGraphicsScene *oldScene = m_panned->scene();
    m_panned->setScene(scene);
    m_panner->setScene(scene);

    if (oldScene) delete oldScene;
    if (tipItem) tipItem->ensureVisible();

    updateCurrentItems();
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

void HistoryWidget::updateCurrentItems()
{
    QGraphicsScene *scene = m_panned->scene();
    if (!scene) return;
    QList<QGraphicsItem *> items = scene->items();
    foreach (QGraphicsItem *it, items) {
        ChangesetItem *csit = dynamic_cast<ChangesetItem *>(it);
        if (csit) {
            QString id = csit->getChangeset()->id();
            bool current = m_currentIds.contains(id);
            if (current) {
                DEBUG << "id " << id << " is current" << endl;
            }
            csit->setCurrent(current);
        }
    }
}

