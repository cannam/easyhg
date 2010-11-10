
#include "grapher.h"

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
    if (!m_idCsetMap.contains(id)) {
	throw LayoutException(QString("Changeset %1 not in ID map").arg(id));
    }
    if (!m_items.contains(id)) {
	throw LayoutException(QString("Changeset %1 not in item map").arg(id));
    }
    Changeset *cs = m_idCsetMap[id];
    ChangesetItem *item = m_items[id];
    std::cerr << "Looking at " << id.toStdString() << std::endl;

    int row = 0;
    int nparents = cs->parents().size();

    if (nparents > 0) {
	bool haveRow = false;
	foreach (QString parentId, cs->parents()) {

	    if (!m_idCsetMap.contains(parentId)) continue;
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
    item->setY(row * 100);
    m_handled.insert(id);
}

void
Grapher::layoutCol(QString id)
{
    if (m_handled.contains(id)) {
	std::cerr << "Already looked at " << id.toStdString() << std::endl;
	return;
    }
    if (!m_idCsetMap.contains(id)) {
	throw LayoutException(QString("Changeset %1 not in ID map").arg(id));
    }
    if (!m_items.contains(id)) {
	throw LayoutException(QString("Changeset %1 not in item map").arg(id));
    }
    Changeset *cs = m_idCsetMap[id];
    ChangesetItem *item = m_items[id];
    std::cerr << "Looking at " << id.toStdString() << std::endl;

    int col = 0;
    int nparents = cs->parents().size();

    if (nparents > 0) {
	bool preferParentCol = true;
	foreach (QString parentId, cs->parents()) {

	    if (!m_idCsetMap.contains(parentId)) continue;
	    if (!m_items.contains(parentId)) continue;

	    if (nparents == 1) {
		// when introducing a new branch, aim _not_ to
		// position child on the same column as parent
		Changeset *parent = m_idCsetMap[parentId];
		if (parent->branch() != cs->branch()) {
		    preferParentCol = false;
		}
	    }		

	    if (!m_handled.contains(parentId)) {
		layoutCol(parentId);
	    }

	    ChangesetItem *parentItem = m_items[parentId];
	    col += parentItem->column();
	}

	col /= cs->parents().size();
	col = findAvailableColumn(item->row(), col, preferParentCol);
	m_alloc[item->row()].insert(col);
    }

    std::cerr << "putting " << cs->id().toStdString() << " at col " << col << std::endl;

    item->setColumn(col);
    item->setX(col * 100);
    m_handled.insert(id);
}

void
Grapher::layout(Changesets csets)
{
    m_idCsetMap.clear();
    m_items.clear();
    m_alloc.clear();

    foreach (Changeset *cs, csets) {

	QString id = cs->id();
	std::cerr << id.toStdString() << std::endl;

	if (id == "") {
	    throw LayoutException("Changeset has no ID");
	}
	if (m_idCsetMap.contains(id)) {
	    throw LayoutException(QString("Duplicate changeset ID %1").arg(id));
	}

	m_idCsetMap[id] = cs;

        ChangesetItem *item = new ChangesetItem(cs);
        item->setX(0);
        item->setY(0);
	m_items[id] = item;
        m_scene->addItem(item);
    }

    // Layout in reverse order, i.e. forward chronological order.
    // This ensures that parents will normally be laid out before
    // their children -- though we can recurse from layout() if we
    // find any weird exceptions
    m_handled.clear();
    for (int i = csets.size() - 1; i >= 0; --i) {
	layoutRow(csets[i]->id());
    }
    m_handled.clear();
    for (int i = csets.size() - 1; i >= 0; --i) {
	layoutCol(csets[i]->id());
    }

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
}

