#ifndef CHANGESETITEM_H
#define CHANGESETITEM_H

#include <QGraphicsItem>
#include <QFont>

class Changeset;

class ChangesetItem : public QGraphicsItem
{
public:
    ChangesetItem(Changeset *cs);

    virtual QRectF boundingRect() const;
    virtual void paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget *);

    Changeset *getChangeset() { return m_changeset; }

    int column() const { return m_column; }
    int row() const { return m_row; }
    void setColumn(int c) { m_column = c; setX(c * 100); }
    void setRow(int r) { m_row = r; setY(r * 90); }

    bool isWide() const { return m_wide; }
    void setWide(bool w) { m_wide = w; }

private:
    QFont m_font;
    Changeset *m_changeset;
    int m_column;
    int m_row;
    bool m_wide;
};

#endif // CHANGESETITEM_H
