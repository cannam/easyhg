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
    
void HistoryWidget::parseLog(QString log)
{
    QGraphicsScene *scene = new QGraphicsScene();
    Changesets csets = parseChangeSets(log);
    ChangesetItem *tipItem = 0;

    if (!csets.empty()) {
	Grapher g(scene);
	try {
	    g.layout(csets);
	} catch (std::string s) {
	    std::cerr << "Internal error: Layout failed: " << s << std::endl;
	}
	tipItem = g.getItemFor(csets[0]);
    }

    QGraphicsScene *oldScene = m_panned->scene();
    m_panned->setScene(scene);
    m_panner->setScene(scene);

    if (oldScene) delete oldScene;
    clearChangesets();

    m_changesets = csets;

    if (tipItem) tipItem->ensureVisible();
}

Changesets HistoryWidget::parseChangeSets(QString changeSetsStr)
{
    Changesets csets = Changeset::parseChangesets(changeSetsStr);
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
