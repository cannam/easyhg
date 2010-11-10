#ifndef CONNECTIONITEM_H
#define CONNECTIONITEM_H

#include <QGraphicsItem>

class Connection;

class ChangesetItem;

class ConnectionItem : public QGraphicsItem
{
public:
    ConnectionItem() : m_parent(0), m_child(0) { }

    virtual QRectF boundingRect() const;
    virtual void paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget *);

    //!!! deletion signals from parent/child

    ChangesetItem *parent() { return m_parent; }
    ChangesetItem *child() { return m_child; }

    void setParent(ChangesetItem *p) { m_parent = p; }
    void setChild(ChangesetItem *c) { m_child = c; }

private:
    ChangesetItem *m_parent;
    ChangesetItem *m_child;
};

#endif // CONNECTIONITEM_H
