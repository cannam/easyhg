
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

    // row is now an upper bound on our eventual row (because we want
    // to be above all parents).  But we also want to ensure we are
    // above all nodes that have earlier dates (to the nearest day).
    // m_rowDates maps each row to a date: use that.

    QString date = cs->date();

    // n.b. this relies on the fact that the date component of an ISO
    // date/time sorts correctly in a dictionary sort
    while (m_rowDates.contains(row) && m_rowDates[row] < date) {
	--row;
    }

    // We have already laid out all nodes that have earlier timestamps
    // than this one, so we know (among other things) that we can
    // safely fill in this row has having this date, if it isn't in
    // the map yet (it cannot have an earlier date)

    if (!m_rowDates.contains(row)) {
	m_rowDates[row] = date;
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

    // Parent may have layed out child in the recursive call
    if (m_handled.contains(id)) {
	std::cerr << "Looks like we've dealt with " << id.toStdString() << std::endl;
	return;
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

    // Normally the children will lay out themselves, but we can do
    // a better job in some special cases:

    int nchildren = cs->children().size();

    // look for merging children and make sure nobody
    // is going to overwrite their "merge lines" if they extend further
    // than a single step

    foreach (QString childId, cs->children()) {
        if (!m_changesets.contains(childId)) continue;
        Changeset *child = m_changesets[childId];
        if (child->parents().size() > 1) {
            int childRow = m_items[childId]->row();
            for (int r = row; r > childRow; --r) {
                m_alloc[r].insert(col);
            }
        }
    }

    // look for the case where exactly two children have the same
    // branch as us: split them to a little either side of our position

    if (nchildren > 1) {

	QList<QString> special;
	foreach (QString childId, cs->children()) {
	    if (!m_changesets.contains(childId)) continue;
	    Changeset *child = m_changesets[childId];
	    if (child->branch() == branch &&
		child->parents().size() == 1) {
		special.push_back(childId);
	    }
	}
	if (special.size() == 2) {
	    for (int i = 0; i < 2; ++i) {
		int off = i * 2 - 1; // 0 -> -1, 1 -> 1
		ChangesetItem *it = m_items[special[i]];
		it->setColumn(findAvailableColumn(it->row(), col + off, true));
		m_alloc[it->row()].insert(it->column());
		m_handled.insert(special[i]);
	    }
	}
    }
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
	int home = 3;
	while (taken.contains(home)) {
	    if (home > 0) home = -home;
	    else home = -(home-3);
	}
	m_branchHomes[branch] = home;
    }

    foreach (QString branch, m_branchRanges.keys()) {
	std::cerr << branch.toStdString() << ": " << m_branchRanges[branch].first << " - " << m_branchRanges[branch].second << ", home " << m_branchHomes[branch] << std::endl;
    }
}

static bool
compareChangesetsByDate(Changeset *const &a, Changeset *const &b)
{
    return a->timestamp() < b->timestamp();
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
	    if (!m_changesets.contains(parentId)) continue;
	    Changeset *parent = m_changesets[parentId];
	    parent->addChild(id);
	    ConnectionItem *conn = new ConnectionItem();
	    conn->setChild(item);
	    conn->setParent(m_items[parentId]);
	    m_scene->addItem(conn);
	}
    }

    // We need to lay out the changesets in forward chronological
    // order.  We have no guarantees about the order in which
    // changesets appear in the list -- in a simple repository they
    // will generally be reverse chronological, but that's far from
    // guaranteed.  So, sort explicitly using the date comparator
    // above

    qStableSort(csets.begin(), csets.end(), compareChangesetsByDate);

    m_handled.clear();
    for (int i = csets.size() - 1; i >= 0; --i) {
	layoutRow(csets[i]->id());
    }

    allocateBranchHomes(csets);

    m_handled.clear();
    for (int i = csets.size() - 1; i >= 0; --i) {
	layoutCol(csets[i]->id());
    }
}

