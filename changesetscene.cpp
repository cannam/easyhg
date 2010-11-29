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

#include "changesetscene.h"
#include "changesetitem.h"

ChangesetScene::ChangesetScene()
    : QGraphicsScene(), m_detailShown(0)
{
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

