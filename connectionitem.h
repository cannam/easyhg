#ifndef CONNECTIONITEM_H
#define CONNECTIONITEM_H

#include <QGraphicsItem>

class Connection;

class ChangesetItem;

class ConnectionItem : public QGraphicsItem
{
public:
    enum Type {
	Normal,
	Split,
	Merge
    };

    ConnectionItem() : m_type(Normal), m_parent(0), m_child(0) { }

    virtual QRectF boundingRect() const;
    virtual void paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget *);

    Type connectionType() const { return m_type; }
    void setConnectionType(Type t) { m_type = t; }

    //!!! deletion signals from parent/child

    ChangesetItem *parent() { return m_parent; }
    ChangesetItem *child() { return m_child; }

    void setParent(ChangesetItem *p) { m_parent = p; }
    void setChild(ChangesetItem *c) { m_child = c; }

private:
    Type m_type;
    ChangesetItem *m_parent;
    ChangesetItem *m_child;
};

#endif // CONNECTIONITEM_H
