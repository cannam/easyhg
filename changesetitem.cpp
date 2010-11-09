#include "changesetitem.h"
#include "changeset.h"

#include <QPainter>

QRectF
ChangesetItem::boundingRect() const
{
    int n = m_changeset->number();
    return QRectF(0, 0, 250, 50);
}

void
ChangesetItem::paint(QPainter *paint, const QStyleOptionGraphicsItem *option,
                     QWidget *w)
{
//    paint->drawText(50, 0, m_changeset->comment());
    paint->drawText(5, 15, m_changeset->authorName());
    paint->drawText(5, 30, m_changeset->branch());
    paint->drawRect(QRectF(0, 0, 50, 50));

//    paint->drawRect(QRectF(0, 0, 50, 50));
}
