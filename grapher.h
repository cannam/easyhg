#ifndef GRAPHER_H
#define GRAPHER_H

#include "changeset.h"
#include "changesetitem.h"

#include <QSet>
#include <QMap>

#include <exception>

class Grapher
{
public:
    Grapher(QGraphicsScene *scene) { m_scene = scene; }

    void layout(Changesets csets);

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
    void layoutRow(QString id);
    void layoutCol(QString id);
    int findAvailableColumn(int row, int parent, bool preferParentCol);

    QGraphicsScene *m_scene;

    typedef QMap<QString, Changeset *> IdChangesetMap;
    IdChangesetMap m_idCsetMap;

    typedef QMap<QString, ChangesetItem *> IdItemMap;
    IdItemMap m_items;

    typedef QSet<int> ColumnSet;
    typedef QMap<int, ColumnSet> GridAlloc;
    GridAlloc m_alloc;

    typedef QSet<QString> IdSet;
    IdSet m_handled;
};

#endif 
