#ifndef DATEITEM_H
#define DATEITEM_H

#include <QGraphicsRectItem>

class DateItem : public QGraphicsItem
{
public:
    DateItem() :
	m_minrow(0), m_maxrow(0),
	m_mincol(0), m_maxcol(0),
	m_even(false) {}

    virtual QRectF boundingRect() const;
    virtual void paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget *);

    void setRows(int minrow, int n);
    void setCols(int mincol, int n);

    void setEven(bool e) { m_even = e; }

    QString dateString() const { return m_dateString; }
    void setDateString(QString s) { m_dateString = s; }

private:
    QString m_dateString;
    int m_minrow;
    int m_maxrow;
    int m_mincol;
    int m_maxcol;
    bool m_even;
};

#endif // DATEITEM_H
