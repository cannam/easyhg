
#include "grapher.h"
#include "connectionitem.h"

#include <QGraphicsScene>

#include <iostream>

int
Grapher::findAvailableColumn(int row, int parent, bool preferParentCol)
{
    int col = parent;
    if (preferParentCol) {
	if (!m_alloc[row].contains(col)) {
	    return col;
	}
    }
    while (col > 0) {
	if (!m_alloc[row].contains(--col)) return col;
    }
    while (col < 0) {
	if (!m_alloc[row].contains(++col)) return col;
    }
    col = parent;
    int sign = (col < 0 ? -1 : 1);
    while (1) {
	col += sign;
	if (!m_alloc[row].contains(col)) return col;
    }
}

void
Grapher::layoutRow(QString id)
{
    if (m_handled.contains(id)) {
	return;
    }
    if (!m_changesets.contains(id)) {
	throw LayoutException(QString("Changeset %1 not in ID map").arg(id));
    }
    if (!m_items.contains(id)) {
	throw LayoutException(QString("Changeset %1 not in item map").arg(id));
    }
    Changeset *cs = m_changesets[id];
    ChangesetItem *item = m_items[id];
    std::cerr << "Looking at " << id.toStdString() << std::endl;

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

    std::cerr << "putting " << cs->id().toStdString() << " at row " << row 
	      << std::endl;

    item->setRow(row);
    m_handled.insert(id);
}

void
Grapher::layoutCol(QString id)
{
    if (m_handled.contains(id)) {
	std::cerr << "Already looked at " << id.toStdString() << std::endl;
	return;
    }
    if (!m_changesets.contains(id)) {
	throw LayoutException(QString("Changeset %1 not in ID map").arg(id));
    }
    if (!m_items.contains(id)) {
	throw LayoutException(QString("Changeset %1 not in item map").arg(id));
    }
    Changeset *cs = m_changesets[id];
    ChangesetItem *item = m_items[id];
    std::cerr << "Looking at " << id.toStdString() << std::endl;

    foreach (QString parentId, cs->parents()) {
	if (!m_changesets.contains(parentId)) continue;
	if (!m_handled.contains(parentId)) {
	    layoutCol(parentId);
	}
    }

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
	    m_changesets[parentId]->branch() != branch) {
	    // new branch
	    col = m_branchHomes[branch];
	} else {
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
	    if (m_changesets[parentId]->branch() == branch) {
		ChangesetItem *parentItem = m_items[parentId];
		col += parentItem->column();
		parentsOnSameBranch++;
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

    std::cerr << "putting " << cs->id().toStdString() << " at col " << col << std::endl;

    m_alloc[row].insert(col);
    item->setColumn(col);
    m_handled.insert(id);
}

bool
Grapher::rangesConflict(const Range &r1, const Range &r2)
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

void
Grapher::allocateBranchHomes(Changesets csets)
{
    foreach (Changeset *cs, csets) {
	QString branch = cs->branch();
	ChangesetItem *item = m_items[cs->id()];
	if (!item) continue;
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
	    if (home > 0) home = -home;
	    else home = -(home-2);
	}
	m_branchHomes[branch] = home;
    }

    foreach (QString branch, m_branchRanges.keys()) {
	std::cerr << branch.toStdString() << ": " << m_branchRanges[branch].first << " - " << m_branchRanges[branch].second << ", home " << m_branchHomes[branch] << std::endl;
    }
}

void
Grapher::layout(Changesets csets)
{
    m_changesets.clear();
    m_items.clear();
    m_alloc.clear();
    m_branchHomes.clear();

    foreach (Changeset *cs, csets) {

	QString id = cs->id();
	std::cerr << id.toStdString() << std::endl;

	if (id == "") {
	    throw LayoutException("Changeset has no ID");
	}
	if (m_changesets.contains(id)) {
	    throw LayoutException(QString("Duplicate changeset ID %1").arg(id));
	}

	m_changesets[id] = cs;

        ChangesetItem *item = new ChangesetItem(cs);
        item->setX(0);
        item->setY(0);
	m_items[id] = item;
        m_scene->addItem(item);
    }

    foreach (Changeset *cs, csets) {
	QString id = cs->id();
	ChangesetItem *item = m_items[id];
	foreach (QString parentId, cs->parents()) {
	    if (!m_items.contains(parentId)) continue;
	    ConnectionItem *conn = new ConnectionItem();
	    conn->setChild(item);
	    conn->setParent(m_items[parentId]);
	    m_scene->addItem(conn);
	}
    }

    // Layout in reverse order, i.e. forward chronological order.
    // This ensures that parents will normally be laid out before
    // their children -- though we can recurse from layout() if we
    // find any weird exceptions
    m_handled.clear();
    for (int i = csets.size() - 1; i >= 0; --i) {
	layoutRow(csets[i]->id());
    }

    allocateBranchHomes(csets);

    m_handled.clear();
    for (int i = csets.size() - 1; i >= 0; --i) {
	layoutCol(csets[i]->id());
    }
/*
    foreach (Changeset *cs, csets) {
	QString id = cs->id();
	if (!m_items.contains(id)) continue;
	ChangesetItem *me = m_items[id];
	foreach (QString parentId, cs->parents()) {
	    if (!m_items.contains(parentId)) continue;
	    ChangesetItem *parent = m_items[parentId];
	    QGraphicsLineItem *line = new QGraphicsLineItem;
	    line->setLine(me->x() + 25, me->y() + 50,
			  parent->x() + 25, parent->y());
	    m_scene->addItem(line);
	}
    }
*/
}

