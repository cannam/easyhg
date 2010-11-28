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

#ifndef GRAPHER_H
#define GRAPHER_H

#include "changeset.h"
#include "changesetitem.h"
#include "changesetscene.h"

#include <QSet>
#include <QMap>
#include <QPair>

#include <exception>

class Grapher
{
public:
    Grapher(ChangesetScene *scene) { m_scene = scene; }

    void layout(Changesets csets);

    ChangesetItem *getItemFor(Changeset *cs);

    class LayoutException : public std::exception {
    public:
	LayoutException(QString message) throw() : m_message(message) { }
	virtual ~LayoutException() throw() { }
	virtual const char *what() const throw() {
	    return m_message.toLocal8Bit().data();
	}
    protected:
	QString m_message;
    };

private:
    ChangesetScene *m_scene;

    typedef QMap<QString, Changeset *> IdChangesetMap;
    IdChangesetMap m_changesets;

    typedef QMap<QString, ChangesetItem *> IdItemMap;
    IdItemMap m_items;

    typedef QSet<int> ColumnSet;
    typedef QMap<int, ColumnSet> GridAlloc;
    GridAlloc m_alloc;

    typedef QPair<int, int> Range;
    typedef QMap<QString, Range> BranchRangeMap;
    BranchRangeMap m_branchRanges;

    typedef QMap<QString, int> BranchColumnMap;
    BranchColumnMap m_branchHomes;

    typedef QSet<QString> IdSet;
    IdSet m_handled;

    typedef QMap<int, QString> RowDateMap;
    RowDateMap m_rowDates;

    void layoutRow(QString id);
    void layoutCol(QString id);
    void allocateBranchHomes(Changesets csets);
    bool rangesConflict(const Range &r1, const Range &r2);
    int findAvailableColumn(int row, int parent, bool preferParentCol);
};

#endif 
