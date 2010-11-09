
#include "grapher.h"

#include <QSet>
#include <QMap>

#include <iostream>

typedef QSet<int> ColumnSet;
typedef QMap<int, ColumnSet> GridAlloc;
typedef QMap<QString, Changeset *> IdChangesetMap;
typedef QSet<Changeset *> ChangesetSet;

ChangesetItem *
layout(Changeset *cs,
       IdChangesetMap idCsetMap,
       ChangesetItemMap items,
       GridAlloc &alloc,
       ChangesetSet &handled)
{
    if (!cs) {
	throw std::string("Null Changeset");
    }
    if (!items.contains(cs)) {
	throw std::string("Changeset not in item map");
    }
    ChangesetItem *item = items[cs];
    if (handled.contains(cs)) {
	return item;
    }
    int row = 0;
    int col = 0;
    if (!cs->parents().empty()) {
	bool haveRow = false;
	foreach (QString parentId, cs->parents()) {
	    if (parentId == "") continue; //!!!
	    std::cerr << "recursing to parent \"" << parentId.toStdString() << "\" of \"" << cs->id().toStdString() << "\"" << std::endl;
	    ChangesetItem *parentItem =
		layout(idCsetMap[parentId],
		       idCsetMap,
		       items,
		       alloc,
		       handled);
	    if (!haveRow || parentItem->row() < row) {
		row = parentItem->row();
		haveRow = true;
	    }
	    col += parentItem->column();
	}
	col /= cs->parents().size();
	row = row - 1;
	while (alloc[row].contains(col)) {
	    if (col > 0) col = -col;
	    else col = -col + 1;
	}
	alloc[row].insert(col);
    }	
    item->setColumn(col);
    item->setRow(row);
    item->setX(col * 100);
    item->setY(row * 100);
    handled.insert(cs);
    return item;
}

void
Grapher::layout(Changesets csets, ChangesetItemMap items)
{
    IdChangesetMap idCsetMap;
    foreach (Changeset *cs, csets) {
	std::cerr << cs->id().toStdString() << std::endl;
	if (cs->id() == "") {
	    throw std::string("Changeset has no ID");
	}
	if (idCsetMap.contains(cs->id())) {
	    throw std::string("Changeset ID is already in map");
	}
	idCsetMap[cs->id()] = cs;
    }

    GridAlloc alloc;
    ChangesetSet handled;
    foreach (Changeset *cs, csets) {
	::layout(cs, idCsetMap, items, alloc, handled);
    }
}

