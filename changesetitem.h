#ifndef CHANGESETITEM_H
#define CHANGESETITEM_H

#include <QGraphicsItem>

class Changeset;

class ChangesetItem : public QGraphicsItem
{
public:
    ChangesetItem(Changeset *cs) : m_changeset(cs) { }

    virtual QRectF boundingRect() const;
    virtual void paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget *);

private:
    Changeset *m_changeset;
};

#endif // CHANGESETITEM_H
