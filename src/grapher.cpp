/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    EasyMercurial

    Based on HgExplorer by Jari Korhonen
    Copyright (c) 2010 Jari Korhonen
    Copyright (c) 2013 Chris Cannam
    Copyright (c) 2013 Queen Mary, University of London
    
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "grapher.h"
#include "connectionitem.h"
#include "debug.h"
#include "changesetscene.h"

#include <QSettings>

#include <iostream>

//#define GRAPHER_VERBOSE_DEBUG 1

Grapher::Grapher(ChangesetScene *scene) :
    m_scene(scene)
{
    QSettings settings;
    settings.beginGroup("Presentation");
    m_showDates = (settings.value("dateformat", 0) == 1);
    m_showClosedBranches = (settings.value("showclosedbranches", false).toBool());
}

int Grapher::findAvailableColumn(int row, int parent, bool preferParentCol)
{
    int col = parent;
    if (preferParentCol) {
        if (isAvailable(row, col)) return col;
    }
    while (col > 0) {
        if (isAvailable(row, --col)) return col;
    }
    while (col < 0) {
        if (isAvailable(row, ++col)) return col;
    }
    col = parent;
    int sign = (col < 0 ? -1 : 1);
    while (1) {
        col += sign;
        if (isAvailable(row, col)) return col;
    }
}

bool Grapher::isAvailable(int row, int col)
{
    if (m_alloc.contains(row) && m_alloc[row].contains(col)) return false;
    if (!m_haveAllocatedUncommittedColumn) return true;
    if (!m_uncommitted) return true;
    return !(row <= m_uncommittedParentRow && col == m_uncommitted->column());
}

void Grapher::layoutRow(QString id)
{
    if (m_handled.contains(id)) {
        return;
    }
    if (!m_changesets.contains(id)) {
        throw LayoutException(QString("Changeset %1 not in ID map").arg(id));
    }
    if (!m_items.contains(id)) {
        return;
    }
    Changeset *cs = m_changesets[id];
    ChangesetItem *item = m_items[id];
#ifdef GRAPHER_VERBOSE_DEBUG
    DEBUG << "layoutRow: Looking at " << id.toStdString() << endl;
#endif

    int row = 0;
    int nparents = cs->parents().size();

    if (nparents > 0) {
        bool haveRow = false;
        foreach (QString parentId, cs->parents()) {

            if (!m_changesets.contains(parentId)) continue;
            if (!m_items.contains(parentId)) continue;

            if (!m_handled.contains(parentId)) {
                layoutRow(parentId);
            }

            ChangesetItem *parentItem = m_items[parentId];
            if (!haveRow || parentItem->row() < row) {
                row = parentItem->row();
                haveRow = true;
            }
        }
        row = row - 1;
    }

    // row is now an upper bound on our eventual row (because we want
    // to be above all parents).  But we also want to ensure we are
    // above all nodes that have earlier dates (to the nearest day).
    // m_rowDates maps each row to a date: use that.

    QString date;
    if (m_showDates) {
        date = cs->date();
    } else {
        date = cs->age();
    }
    while (m_rowDates.contains(row) && m_rowDates[row] != date) {
        --row;
    }

    // We have already laid out all nodes that have earlier timestamps
    // than this one, so we know (among other things) that we can
    // safely fill in this row has having this date, if it isn't in
    // the map yet (it cannot have an earlier date)

    if (!m_rowDates.contains(row)) {
        m_rowDates[row] = date;
    }

    // If we're the parent of the uncommitted item, make a note of our
    // row (we need it later, to avoid overwriting the connecting line)
    if (!m_uncommittedParents.empty() && m_uncommittedParents[0] == id) {
        m_uncommittedParentRow = row;
    }

#ifdef GRAPHER_VERBOSE_DEBUG
    DEBUG << "putting " << cs->id().toStdString() << " at row " << row 
          << endl;
#endif
    
    item->setRow(row);
    m_handled.insert(id);
}

void Grapher::layoutCol(QString id)
{
    if (m_handled.contains(id)) {
#ifdef GRAPHER_VERBOSE_DEBUG
        DEBUG << "Already looked at " << id.toStdString() << endl;
#endif
        return;
    }
    if (!m_changesets.contains(id)) {
        throw LayoutException(QString("Changeset %1 not in ID map").arg(id));
    }
    if (!m_items.contains(id)) {
        return;
    }

    Changeset *cs = m_changesets[id];
#ifdef GRAPHER_VERBOSE_DEBUG
    DEBUG << "layoutCol: Looking at " << id.toStdString() << endl;
#endif

    ChangesetItem *item = m_items[id];

    int col = 0;
    int row = item->row();
    QString branch = cs->branch();

    int nparents = cs->parents().size();
    QString parentId;
    int parentsOnSameBranch = 0;

    switch (nparents) {

    case 0:
        col = m_branchHomes[cs->branch()];
        col = findAvailableColumn(row, col, true);
        break;

    case 1:
        parentId = cs->parents()[0];

        if (!m_changesets.contains(parentId) ||
            !m_changesets[parentId]->isOnBranch(branch)) {
            // new branch
            col = m_branchHomes[branch];
        } else if (m_items.contains(parentId)) {
            col = m_items[parentId]->column();
        }

        col = findAvailableColumn(row, col, true);
        break;

    case 2:
        // a merge: behave differently if parents are both on the same
        // branch (we also want to behave differently for nodes that
        // have multiple children on the same branch -- spreading them
        // out rather than having affinity to a specific branch)

        foreach (QString parentId, cs->parents()) {
            if (!m_changesets.contains(parentId)) continue;
            if (m_changesets[parentId]->isOnBranch(branch)) {
                if (m_items.contains(parentId)) {
                    ChangesetItem *parentItem = m_items[parentId];
                    col += parentItem->column();
                    parentsOnSameBranch++;
                }
            }
        }

        if (parentsOnSameBranch > 0) {
            col /= parentsOnSameBranch;
            col = findAvailableColumn(item->row(), col, true);
        } else {
            col = findAvailableColumn(item->row(), col, false);
        }
        break;
    }

#ifdef GRAPHER_VERBOSE_DEBUG
    DEBUG << "putting " << cs->id().toStdString() << " at col " << col << endl;
#endif
    
    m_alloc[row].insert(col);
    item->setColumn(col);
    m_handled.insert(id);

    // If we're the first parent of the uncommitted item, it should be
    // given the same column as us (we already noted that its
    // connecting line would end at our row)

    if (m_uncommittedParents.contains(id)) {
        if (m_uncommittedParents[0] == id) {
            int ucol = findAvailableColumn(row-1, col, true);
            m_uncommitted->setColumn(ucol);
            m_haveAllocatedUncommittedColumn = true;
        }
        // also, if the uncommitted item has a different branch from
        // any of its parents, tell it to show the branch
        if (!cs->isOnBranch(m_uncommitted->branch())) {
            DEBUG << "Uncommitted branch " << m_uncommitted->branch()
                  << " differs from my branch " << cs->branch()
                  << ", asking it to show branch" << endl;
            m_uncommitted->setShowBranch(true);
        }
    }


    // Normally the children will lay out themselves, but we can do
    // a better job in some special cases:

    int nchildren = cs->children().size();

    // look for merging children and children distant from us but in a
    // straight line, and make sure nobody is going to overwrite their
    // connection lines

    foreach (QString childId, cs->children()) {
#ifdef GRAPHER_VERBOSE_DEBUG
        DEBUG << "reserving connection line space" << endl;
#endif
        if (!m_items.contains(childId)) continue;
        Changeset *child = m_changesets[childId];
        int childRow = m_items[childId]->row();
        if (child->parents().size() > 1 ||
            child->isOnBranch(cs->branch())) {
            for (int r = row-1; r > childRow; --r) {
                m_alloc[r].insert(col);
            }
        }
    }

    // look for the case where exactly two children have the same
    // branch as us: split them to a little either side of our position

    if (nchildren > 1) {
        QList<QString> special;
        foreach (QString childId, cs->children()) {
            if (!m_items.contains(childId)) continue;
            Changeset *child = m_changesets[childId];
            if (child->isOnBranch(branch) &&
                child->parents().size() == 1) {
                special.push_back(childId);
            }
        }
        if (special.size() == 2) {
#ifdef GRAPHER_VERBOSE_DEBUG
            DEBUG << "handling split-in-two for children " << special[0] << " and " << special[1] << endl;
#endif
            for (int i = 0; i < 2; ++i) {
                int off = i * 2 - 1; // 0 -> -1, 1 -> 1
                ChangesetItem *it = m_items[special[i]];
                it->setColumn(findAvailableColumn(it->row(), col + off, true));
                for (int r = row-1; r >= it->row(); --r) {
                    m_alloc[r].insert(it->column());
                }
                m_handled.insert(special[i]);
            }
        }
    }
}

bool Grapher::rangesConflict(const Range &r1, const Range &r2)
{
    // allow some additional space at edges.  we really ought also to
    // permit space at the end of a branch up to the point where the
    // merge happens
    int a1 = r1.first - 2, b1 = r1.second + 2;
    int a2 = r2.first - 2, b2 = r2.second + 2;
    if (a1 > b2 || b1 < a2) return false;
    if (a2 > b1 || b2 < a1) return false;
    return true;
}

void Grapher::allocateBranchHomes(Changesets csets)
{
    foreach (Changeset *cs, csets) {
        QString id = cs->id();
        if (!m_items.contains(id)) continue;
        ChangesetItem *item = m_items[id];
        QString branch = cs->branch();
        int row = item->row();
        if (!m_branchRanges.contains(branch)) {
            m_branchRanges[branch] = Range(row, row);
        } else {
            Range p = m_branchRanges[branch];
            if (row < p.first) p.first = row;
            if (row > p.second) p.second = row;
            m_branchRanges[branch] = p;
        }
    }

    m_branchHomes[""] = 0;
    m_branchHomes["default"] = 0;

    foreach (QString branch, m_branchRanges.keys()) {
        if (branch == "") continue;
        QSet<int> taken;
        taken.insert(0);
        Range myRange = m_branchRanges[branch];
        foreach (QString other, m_branchRanges.keys()) {
            if (other == branch || other == "") continue;
            Range otherRange = m_branchRanges[other];
            if (rangesConflict(myRange, otherRange)) {
                if (m_branchHomes.contains(other)) {
                    taken.insert(m_branchHomes[other]);
                }
            }
        }
        int home = 2;
        while (taken.contains(home)) {
            if (home > 0) {
                if (home % 2 == 1) {
                    home = -home;
                } else {
                    home = home + 1;
                }
            } else {
                if ((-home) % 2 == 1) {
                    home = home + 1;
                } else {
                    home = -(home-2);
                }
            }
        }
        m_branchHomes[branch] = home;
    }

#ifdef GRAPHER_VERBOSE_DEBUG
    foreach (QString branch, m_branchRanges.keys()) {
        DEBUG << branch.toStdString() << ": " << m_branchRanges[branch].first << " - " << m_branchRanges[branch].second << ", home " << m_branchHomes[branch] << endl;
    }
#endif
}

static bool
compareChangesetsByDate(Changeset *const &a, Changeset *const &b)
{
    return a->timestamp() < b->timestamp();
}

ChangesetItem *
Grapher::getItemFor(Changeset *cs)
{
    if (!cs) return 0;
    return getItemFor(cs->id());
}

ChangesetItem *
Grapher::getItemFor(QString id)
{
    if (!m_items.contains(id)) return 0;
    return m_items[id];
}

void
Grapher::markClosedChangesets()
{
    // Ensure the closed branch changesets are all marked as closed.

    QSet<QString> deferred;

    foreach (QString id, m_closedIds) {
        markClosedChangesetsFrom(id, deferred);
//        std::cerr << "after closed id " << id << ": candidates now contains " << deferred.size() << " element(s)" << std::endl;
    }

    while (!deferred.empty()) {
        foreach (QString id, deferred) {
            markClosedChangesetsFrom(id, deferred);
            deferred.remove(id);
//        std::cerr << "after id " << id << ": deferred now contains " << deferred.size() << " element(s)" << std::endl;
        }
    }
}

void
Grapher::markClosedChangesetsFrom(QString id, QSet<QString> &deferred)
{
    // A changeset should be marked as closed (i) if it is in the list
    // of closed heads [and has no children]; or (ii) all of its
    // children that have the same branch name as it are marked as
    // closed [and there is at least one of those]

    if (!m_changesets.contains(id)) {
//        std::cerr << "no good" << std::endl;
        return;
    }

//    std::cerr << "looking at id " << id << std::endl;
            
    Changeset *cs = m_changesets[id];
    QString branch = cs->branch();
            
    bool closed = false;

    if (m_closedIds.contains(id)) {

        closed = true;

    } else {

        closed = false;
        foreach (QString childId, cs->children()) {
            if (!m_changesets.contains(childId)) {
                continue;
            }
            Changeset *ccs = m_changesets[childId];
            if (ccs->isOnBranch(branch)) {
                if (ccs->closed()) {
                    // closed becomes true only when we see a closed
                    // child on the same branch
                    closed = true;
                } else {
                    // and it becomes false as soon as we see any
                    // un-closed child on the same branch
                    closed = false;
                    break;
                }
            }
        }
    }

    if (closed) {
        // set closed on this cset and its direct simple parents
        QString csid = id;
        while (cs) {
            cs->setClosed(true);
            if (cs->parents().size() == 1) {
                QString pid = cs->parents()[0];
                if (!m_changesets.contains(pid)) break;
                cs = m_changesets[pid];
                if (cs->children().size() > 1) {
//                    std::cerr << "adding pid " << pid << " (it has more than one child)" << std::endl;
                    deferred.insert(pid); // examine later
                    cs = 0;
                }
            } else if (cs->parents().size() > 1) {
                foreach (QString pid, cs->parents()) {
//                    std::cerr << "recursing to pid " << pid << " (it is one of multiple parents)" << std::endl;
                    markClosedChangesetsFrom(pid, deferred);
                }
                cs = 0;
            } else {
                cs = 0;
            }
        }
    } else {
        cs->setClosed(false);
    }
    
//    std::cerr << "finished with id " << id << std::endl;
}

void Grapher::layout(Changesets csets,
                     QStringList uncommittedParents,
                     QString uncommittedBranch)
{
    m_changesets.clear();
    m_items.clear();
    m_alloc.clear();
    m_branchHomes.clear();

    m_uncommittedParents = uncommittedParents;
    m_haveAllocatedUncommittedColumn = false;
    m_uncommittedParentRow = 0;
    m_uncommitted = 0;

    DEBUG << "Grapher::layout: Have " << csets.size() << " changesets" << endl;

    if (csets.empty()) return;

    // Initialise changesets hash

    foreach (Changeset *cs, csets) {

        QString id = cs->id();

        if (id == "") {
            throw LayoutException("Changeset has no ID");
        }
        if (m_changesets.contains(id)) {
            DEBUG << "Duplicate changeset ID " << id
                  << " in Grapher::layout()" << endl;
            throw LayoutException(QString("Duplicate changeset ID %1").arg(id));
        }

        m_changesets[id] = cs;
    }
    
    // Set the children for each changeset

    foreach (Changeset *cs, csets) {
        QString id = cs->id();
        foreach (QString parentId, cs->parents()) {
            if (!m_changesets.contains(parentId)) continue;
            Changeset *parent = m_changesets[parentId];
            parent->addChild(id);
        }
    }
    
    // Ensure the closed branch changesets are all marked as closed.

    markClosedChangesets();

    // Create (but don't yet position) the changeset items

    foreach (Changeset *cs, csets) {
        if (cs->closed() && !m_showClosedBranches) continue;
        QString id = cs->id();
        ChangesetItem *item = new ChangesetItem(cs);
        item->setX(0);
        item->setY(0);
        item->setZValue(0);
        m_items[id] = item;
        m_scene->addChangesetItem(item);
    }
    
    // Ensure the closing changeset items are appropriately marked

    foreach (QString closedId, m_closedIds) {
        if (!m_items.contains(closedId)) continue;
        m_items[closedId]->setClosingCommit(true);
    }

    // Add the connecting lines

    foreach (Changeset *cs, csets) {
        QString id = cs->id();
        if (!m_items.contains(id)) continue;
        ChangesetItem *item = m_items[id];
        bool merge = (cs->parents().size() > 1);
        foreach (QString parentId, cs->parents()) {
            if (!m_changesets.contains(parentId)) continue;
            ConnectionItem *conn = new ConnectionItem();
            if (merge) conn->setConnectionType(ConnectionItem::Merge);
            conn->setChild(item);
            conn->setZValue(-1);
            if (m_items.contains(parentId)) {
                conn->setParent(m_items[parentId]);
            } else {
                conn->setMergedBranch(m_changesets[parentId]->branch());
            }
            m_scene->addItem(conn);
        }
    }

    // Add uncommitted item and connecting line as necessary

    if (!m_uncommittedParents.empty()) {

        m_uncommitted = new UncommittedItem();
        m_uncommitted->setBranch(uncommittedBranch);
        m_uncommitted->setZValue(10);
        m_scene->addUncommittedItem(m_uncommitted);

        bool haveParentOnBranch = false;
        foreach (QString p, m_uncommittedParents) {
            if (!m_items.contains(p)) continue;
            ConnectionItem *conn = new ConnectionItem();
            conn->setConnectionType(ConnectionItem::Merge);
            ChangesetItem *pitem = m_items[p];
            conn->setParent(pitem);
            conn->setChild(m_uncommitted);
            conn->setZValue(-1);
            m_scene->addItem(conn);
            if (pitem) {
                if (pitem->getChangeset()->isOnBranch(uncommittedBranch)) {
                    haveParentOnBranch = true;
                }
            }
        }

        // If the uncommitted item has no parents on the same branch,
        // tell it it has a new branch (the "show branch" flag is set
        // elsewhere for this item)
        m_uncommitted->setIsNewBranch(!haveParentOnBranch);

        // Uncommitted is a merge if it has more than one parent
        m_uncommitted->setIsMerge(m_uncommittedParents.size() > 1);
    }

    // Add the branch labels

    foreach (Changeset *cs, csets) {
        QString id = cs->id();
        if (!m_items.contains(id)) continue;
        ChangesetItem *item = m_items[id];
        bool haveChildOnSameBranch = false;
        foreach (QString childId, cs->children()) {
            Changeset *child = m_changesets[childId];
            if (child->branch() == cs->branch()) {
                haveChildOnSameBranch = true;
                break;
            }
        }
        item->setShowBranch(!haveChildOnSameBranch);
    }

    // We need to lay out the changesets in forward chronological
    // order.  We have no guarantees about the order in which
    // changesets appear in the list -- in a simple repository they
    // will generally be reverse chronological, but that's far from
    // guaranteed.  So, sort explicitly using the date comparator
    // above

    qStableSort(csets.begin(), csets.end(), compareChangesetsByDate);

#ifdef GRAPHER_VERBOSE_DEBUG
    foreach (Changeset *cs, csets) {
        DEBUG << "id " << cs->id().toStdString() << ", ts " << cs->timestamp()
              << ", date " << cs->datetime().toStdString() << endl;
    }
#endif

    m_handled.clear();
    foreach (Changeset *cs, csets) {
        layoutRow(cs->id());
    }

    allocateBranchHomes(csets);

    m_handled.clear();
    foreach (Changeset *cs, csets) {
        foreach (QString parentId, cs->parents()) {
            if (!m_handled.contains(parentId) &&
                m_changesets.contains(parentId)) {
                layoutCol(parentId);
            }
        }
        layoutCol(cs->id());
    }

    // Find row and column extents.  We know that 0 is an upper bound
    // on row, and that mincol must be <= 0 and maxcol >= 0, so these
    // initial values are good

    int minrow = 0, maxrow = 0;
    int mincol = 0, maxcol = 0;

    foreach (int r, m_alloc.keys()) {
        if (r < minrow) minrow = r;
        if (r > maxrow) maxrow = r;
        ColumnSet &c = m_alloc[r];
        foreach (int i, c) {
            if (i < mincol) mincol = i;
            if (i > maxcol) maxcol = i;
        }
    }

    int datemincol = mincol, datemaxcol = maxcol;

    if (mincol == maxcol) {
        --datemincol;
        ++datemaxcol;
    } else if (m_alloc[minrow].contains(mincol)) {
        --datemincol;
    }

    // We've given the uncommitted item a column, but not a row yet --
    // it always goes at the top

    if (m_uncommitted) {
        --minrow;
#ifdef GRAPHER_VERBOSE_DEBUG
        DEBUG << "putting uncommitted item at row " << minrow << endl;
#endif
        m_uncommitted->setRow(minrow);
    }

    // Changeset items that have nothing to either side of them can be
    // made double-width

    foreach (Changeset *cs, csets) {
        QString id = cs->id();
        if (!m_items.contains(id)) continue;
        ChangesetItem *item = m_items[id];
        if (isAvailable(item->row(), item->column()-1) &&
            isAvailable(item->row(), item->column()+1)) {
            item->setWide(true);
        }
    }

    if (m_uncommitted) {
        if (isAvailable(m_uncommitted->row(), m_uncommitted->column()-1) &&
            isAvailable(m_uncommitted->row(), m_uncommitted->column()+1)) {
            m_uncommitted->setWide(true);
        }
    }

    QString prevDate;
    int changeRow = 0;

    bool even = false;
    int n = 0;

    for (int row = minrow; row <= maxrow; ++row) {

        QString date = m_rowDates[row];
        n++;

        if (date != prevDate) {
            if (prevDate != "") {
                m_scene->addDateRange(prevDate, changeRow, n, even);
                even = !even;
            }
            prevDate = date;
            changeRow = row;
            n = 0;
        }
    }
    
    if (n > 0) {
        m_scene->addDateRange(prevDate, changeRow, n+1, even);
        even = !even;
    }

    m_scene->itemAddCompleted();
}

