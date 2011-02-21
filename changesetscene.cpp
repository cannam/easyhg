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

#include "changesetscene.h"
#include "changesetitem.h"
#include "uncommitteditem.h"
#include "dateitem.h"

ChangesetScene::ChangesetScene()
    : QGraphicsScene(), m_detailShown(0)
{
}

void
ChangesetScene::addChangesetItem(ChangesetItem *item)
{
    addItem(item);

    connect(item, SIGNAL(detailShown()),
            this, SLOT(changesetDetailShown()));

    connect(item, SIGNAL(detailHidden()),
            this, SLOT(changesetDetailHidden()));

    connect(item, SIGNAL(updateTo(QString)),
            this, SIGNAL(updateTo(QString)));

    connect(item, SIGNAL(diffToCurrent(QString)),
            this, SIGNAL(diffToCurrent(QString)));

    connect(item, SIGNAL(diffToParent(QString, QString)),
            this, SIGNAL(diffToParent(QString, QString)));

    connect(item, SIGNAL(showSummaryToParent(QString, QString)),
            this, SIGNAL(showSummaryToParent(QString, QString)));

    connect(item, SIGNAL(mergeFrom(QString)),
            this, SIGNAL(mergeFrom(QString)));

    connect(item, SIGNAL(tag(QString)),
            this, SIGNAL(tag(QString)));
}

void
ChangesetScene::addUncommittedItem(UncommittedItem *item)
{
    addItem(item);
    
    connect(item, SIGNAL(commit()),
            this, SIGNAL(commit()));
    
    connect(item, SIGNAL(revert()),
            this, SIGNAL(revert()));
    
    connect(item, SIGNAL(diff()),
            this, SIGNAL(diffWorkingFolder()));

    connect(item, SIGNAL(showSummary()),
            this, SIGNAL(showSummary()));

    connect(item, SIGNAL(showWork()),
            this, SIGNAL(showWork()));
}

void
ChangesetScene::addDateItem(DateItem *item)
{
    addItem(item);

    connect(item, SIGNAL(clicked()),
            this, SLOT(dateItemClicked()));
}

void
ChangesetScene::changesetDetailShown()
{
    ChangesetItem *csi = qobject_cast<ChangesetItem *>(sender());
    if (!csi) return;

    if (m_detailShown && m_detailShown != csi) {
	m_detailShown->hideDetail();
    }
    m_detailShown = csi;
}

void
ChangesetScene::changesetDetailHidden()
{
    m_detailShown = 0;
}

void
ChangesetScene::dateItemClicked()
{
    if (m_detailShown) {
        m_detailShown->hideDetail();
    }
}

ChangesetItem *
ChangesetScene::getItemById(QString id)
{
    foreach (QGraphicsItem *it, items()) {
        ChangesetItem *csit = dynamic_cast<ChangesetItem *>(it);
        if (csit && csit->getId() == id) return csit;
    }
    return 0;
}


