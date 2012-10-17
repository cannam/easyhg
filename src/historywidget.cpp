/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    EasyMercurial

    Based on HgExplorer by Jari Korhonen
    Copyright (c) 2010 Jari Korhonen
    Copyright (c) 2012 Chris Cannam
    Copyright (c) 2012 Queen Mary, University of London
    
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "historywidget.h"

#include "changesetscene.h"
#include "changesetview.h"
#include "panner.h"
#include "grapher.h"
#include "debug.h"
#include "uncommitteditem.h"
#include "findwidget.h"

#include <iostream>

#include <QGridLayout>
#include <QSettings>

HistoryWidget::HistoryWidget() :
    m_showUncommitted(false),
    m_refreshNeeded(false)
{
    m_panned = new ChangesetView;
    m_panner = new Panner;

    m_panned->setDragMode(QGraphicsView::ScrollHandDrag);
    m_panned->setViewportUpdateMode(QGraphicsView::BoundingRectViewportUpdate);
    m_panned->setCacheMode(QGraphicsView::CacheNone);

    int row = 0;

    QGridLayout *layout = new QGridLayout;
    layout->setMargin(10);
    layout->addWidget(m_panned, row, 0);
    layout->addWidget(m_panner, row, 1);
    m_panner->setMaximumWidth(80);
    m_panner->connectToPanned(m_panned);

    layout->setRowStretch(row, 20);

    QSettings settings;
    settings.beginGroup("Presentation");
    bool showClosed = (settings.value("showclosedbranches", false).toBool());

    QWidget *opts = new QWidget;
    QGridLayout *optLayout = new QGridLayout(opts);
    optLayout->setMargin(0);
    layout->addWidget(opts, ++row, 0, 1, 2);

    m_findWidget = new FindWidget(this);
    optLayout->addWidget(m_findWidget, 0, 0, Qt::AlignLeft);
    connect(m_findWidget, SIGNAL(findTextChanged(QString)),
            this, SLOT(setSearchText(QString)));

    m_showClosedBranches = new QCheckBox(tr("Show closed branches"), this);
    m_showClosedBranches->setChecked(showClosed);
    connect(m_showClosedBranches, SIGNAL(toggled(bool)), 
            this, SLOT(showClosedChanged(bool)));
    optLayout->addWidget(m_showClosedBranches, 0, 1, Qt::AlignRight);
    //    m_showClosedBranches->hide();

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

void HistoryWidget::setClosedHeadIds(QSet<QString> closed)
{
    if (closed == m_closedIds) return;
    m_closedIds = closed;
    m_showClosedBranches->setVisible(!closed.empty());
    m_refreshNeeded = true;
}

void HistoryWidget::setShowUncommitted(bool showUncommitted)
{
    setCurrent(m_currentIds, m_currentBranch, showUncommitted);
}

void HistoryWidget::showClosedChanged(bool show)
{
    QSettings settings;
    settings.beginGroup("Presentation");
    settings.setValue("showclosedbranches", show);
    layoutAll();
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

void HistoryWidget::clear()
{
    QGraphicsScene *oldScene = m_panned->scene();
    m_panned->setScene(0);
    delete oldScene;
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
        g.setClosedHeadIds(m_closedIds);
	try {
	    g.layout(m_changesets,
                     m_showUncommitted ? m_currentIds : QStringList(),
                     m_currentBranch);
	} catch (std::string s) {
	    std::cerr << "Internal error: Layout failed: " << s << std::endl;
	}
        toFocus = g.getUncommittedItem();
        if (!toFocus) {
            if (!m_currentIds.empty()) {
                for (int i = 0; i < m_currentIds.size(); ++i) {
                    toFocus = g.getItemFor(m_currentIds[i]);
                    if (toFocus != 0) {
                        break;
                    }
                }
            } else {
                toFocus = g.getItemFor(m_changesets[0]);
            }
        }
        if (!toFocus) {
            for (int i = 0; i < m_changesets.size(); ++i) {
                toFocus = g.getItemFor(m_changesets[i]);
                if (toFocus != 0) {
                    break;
                }
            }
        }
    }

    m_panned->setScene(scene);
    m_panner->setScene(scene);

    updateNewAndCurrentItems();

    if (toFocus) {
        toFocus->ensureVisible();
    }

    if (m_searchText != "") {
        updateSearchStatus();
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

        if (csit->isCurrent() != current ||
            csit->isNew() != newid) {
            csit->setCurrent(current);
            csit->setNew(newid);
            csit->update();
        }
    }
}

void HistoryWidget::setSearchText(QString text)
{
    if (m_searchText == text) return;
    m_searchText = text;
    updateSearchStatus();
}

void HistoryWidget::updateSearchStatus()
{
    QGraphicsScene *scene = m_panned->scene();
    if (!scene) return;

    ChangesetItem *toFocus = 0;

    QList<QGraphicsItem *> items = scene->items();
    foreach (QGraphicsItem *it, items) {

        ChangesetItem *csit = dynamic_cast<ChangesetItem *>(it);
        if (!csit) continue;
        
        bool matched = csit->matchSearchText(m_searchText);
        if (matched && (!toFocus || csit->row() < toFocus->row())) {
            toFocus = csit;
        }
        csit->update();
    }

    if (toFocus) {
        toFocus->ensureVisible();
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

    connect(scene, SIGNAL(newBranch()),
            this, SIGNAL(newBranch()));

    connect(scene, SIGNAL(noBranch()),
            this, SIGNAL(noBranch()));
    
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

    connect(scene, SIGNAL(closeBranch(QString)),
            this, SIGNAL(closeBranch(QString)));

    connect(scene, SIGNAL(tag(QString)),
            this, SIGNAL(tag(QString)));
}
