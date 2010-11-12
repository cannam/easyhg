#include "changesetitem.h"
#include "changeset.h"
#include "textabbrev.h"
#include "colourset.h"

#include <QPainter>

ChangesetItem::ChangesetItem(Changeset *cs) :
    m_changeset(cs), m_column(0), m_row(0), m_wide(false)
{
    m_font = QFont();
    m_font.setPixelSize(11);
    m_font.setBold(false);
    m_font.setItalic(false);
}

QRectF
ChangesetItem::boundingRect() const
{
    int w = 100;
    if (m_wide) w = 180;
    return QRectF(-((w-50)/2 - 1), -30, w - 3, 79);
}

void
ChangesetItem::paint(QPainter *paint, const QStyleOptionGraphicsItem *option,
                     QWidget *w)
{
    paint->save();
    
    ColourSet *colourSet = ColourSet::instance();
    QColor branchColour = colourSet->getColourFor(m_changeset->branch());
    QColor userColour = colourSet->getColourFor(m_changeset->author());

    QFont f(m_font);

    QTransform t = paint->worldTransform();
    float scale = std::min(t.m11(), t.m22());
    if (scale > 1.0) {
	int ps = int((f.pixelSize() / scale) + 0.5);
	if (ps < 8) ps = 8;
	f.setPixelSize(ps);
    }

    if (scale < 0.1) {
	paint->setPen(QPen(branchColour, 0));
    } else {
	paint->setPen(QPen(branchColour, 2));
    }
	
    paint->setFont(f);
    QFontMetrics fm(f);
    int fh = fm.height();

    int width = 100;
    if (m_wide) width = 180;
    int x0 = -((width - 50) / 2 - 1);

    QRectF r(x0, 0, width - 3, 49);
    paint->drawRect(r);

    if (scale < 0.1) {
	paint->restore();
	return;
    }

    if (m_changeset->children().empty()) {
	f.setBold(true);
	paint->setFont(f);
	QString branch = m_changeset->branch();
	int wid = width - 3;
	branch = TextAbbrev::abbreviate(branch, QFontMetrics(f), wid);
	paint->drawText(x0, -fh + fm.ascent() - 4, branch);
	f.setBold(false);
    }

    paint->fillRect(QRectF(x0 + 0.5, 0.5, width - 4, fh - 0.5),
		    QBrush(userColour));

    paint->setPen(QPen(Qt::white));

    int wid = width - 5;
    QString person = TextAbbrev::abbreviate(m_changeset->authorName(), fm, wid);
    paint->drawText(x0 + 3, fm.ascent(), person);

    paint->setPen(QPen(Qt::black));

    f.setItalic(true);
    fm = QFontMetrics(f);
    fh = fm.height();
    paint->setFont(f);

    QString comment = m_changeset->comment().trimmed();
    comment = comment.replace("\\n", "\n");
    comment = comment.replace(QRegExp("^\"\\s*\\**\\s*"), "");
    comment = comment.replace(QRegExp("\"$"), "");
    comment = comment.replace("\\\"", "\"");
    comment = comment.split('\n')[0];

    wid = width - 5;
    comment = TextAbbrev::abbreviate(comment, fm, wid, TextAbbrev::ElideEnd,
				     "...", 2);

    QStringList lines = comment.split('\n');
    for (int i = 0; i < lines.size(); ++i) {
	paint->drawText(x0 + 3, i * fh + fh + fm.ascent(), lines[i].trimmed());
    }

    paint->restore();
}
