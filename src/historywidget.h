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

#ifndef HISTORYWIDGET_H
#define HISTORYWIDGET_H

#include "changeset.h"

#include <QWidget>
#include <QSet>
#include <QCheckBox>

class Panned;
class Panner;
class UncommittedItem;
class QGraphicsScene;

class HistoryWidget : public QWidget
{
    Q_OBJECT

public:
    HistoryWidget();
    virtual ~HistoryWidget();

    void setCurrent(QStringList ids, QString branch, bool showUncommitted);
    void setShowUncommitted(bool showUncommitted);

    void setClosedHeadIds(QSet<QString> closed);

    void parseNewLog(QString log);
    void parseIncrementalLog(QString log);

    bool haveNewItems() const { return !m_newIds.empty(); }

    void update();

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
    void showClosedChanged(bool);
    
private:
    Changesets m_changesets;
    QStringList m_currentIds;
    QString m_currentBranch;
    QSet<QString> m_newIds;
    QSet<QString> m_closedIds;
    bool m_showUncommitted;
    bool m_refreshNeeded;

    Panned *m_panned;
    Panner *m_panner;
    QCheckBox *m_showClosedBranches;

    QGraphicsScene *scene();
    void clearChangesets();
    void replaceChangesets(Changesets);
    void addChangesets(Changesets);
    void layoutAll();
    void setChangesetParents();
    void updateNewAndCurrentItems();
    void connectSceneSignals();
};

#endif
