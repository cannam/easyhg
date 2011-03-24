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

#ifndef CHANGESETSCENE_H
#define CHANGESETSCENE_H

#include <QGraphicsScene>

class ChangesetItem;
class Changeset;
class UncommittedItem;
class DateItem;

class ChangesetScene : public QGraphicsScene
{
    Q_OBJECT

public:
    ChangesetScene();

    void addChangesetItem(ChangesetItem *item);
    void addUncommittedItem(UncommittedItem *item);
    void addDateItem(DateItem *item);

    ChangesetItem *getItemById(QString id); // Slow: traversal required

signals:
    void commit();
    void revert();
    void diffWorkingFolder();
    void showSummary();
    void showWork();
    void newBranch();
    void noBranch();

    void updateTo(QString id);
    void diffToParent(QString id, QString parent);
    void showSummary(Changeset *);
    void diffToCurrent(QString id);
    void mergeFrom(QString id);
    void newBranch(QString id);
    void tag(QString id);

private slots:
    void changesetDetailShown();
    void changesetDetailHidden();
    void dateItemClicked();

private:
    ChangesetItem *m_detailShown;
};

#endif