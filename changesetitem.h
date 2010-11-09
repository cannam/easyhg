#ifndef CHANGESETITEM_H
#define CHANGESETITEM_H

#include <QGraphicsItem>

class Changeset;

class ChangesetItem : public QGraphicsItem
{
public:
    ChangesetItem(Changeset *cs) : m_changeset(cs), m_column(0), m_row(0) { }

    virtual QRectF boundingRect() const;
    virtual void paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget *);

    int column() const { return m_column; }
    int row() const { return m_row; }
    void setColumn(int c) { m_column = c; }
    void setRow(int r) { m_row = r; }

private:
    Changeset *m_changeset;
    int m_column;
    int m_row;
};

#endif // CHANGESETITEM_H
